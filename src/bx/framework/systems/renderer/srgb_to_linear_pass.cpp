#include "bx/framework/systems/renderer/srgb_to_linear_pass.hpp"

#include "bx/engine/containers/lazy_init.hpp"

const char* SRGB_TO_LINEAR_SHADER_SRC = R""""(

layout (binding = 0) uniform sampler2D InTexture;
layout (binding = 1, rgba8) uniform image2D OutTexture;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dispatchSize = ivec2(gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
    if (id.x >= dispatchSize.x || id.y >= dispatchSize.y) return;

    vec2 uv = vec2(id) / vec2(dispatchSize);

    vec4 color = texture(InTexture, uv);
    imageStore(OutTexture, id, color);
}

)"""";

struct SrgbToLinearPipeline : public LazyInit<SrgbToLinearPipeline, ComputePipelineHandle>
{
    SrgbToLinearPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Srgb to Linear Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = SRGB_TO_LINEAR_SHADER_SRC;
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),                                         // layout (binding = 0, std140) uniform Constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA8_UNORM)),   // layout (binding = 1, std140) uniform Model
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Srgb to Linear Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};

std::unique_ptr<SrgbToLinearPipeline> LazyInit<SrgbToLinearPipeline, ComputePipelineHandle>::cache = nullptr;

SrgbToLinearPass::SrgbToLinearPass(TextureHandle srgbTexture, TextureHandle linearTexture)
{
    const TextureCreateInfo& srgbCreateInfo = Graphics::GetTextureCreateInfo(srgbTexture);
    const TextureCreateInfo& linearCreateInfo = Graphics::GetTextureCreateInfo(linearTexture);
    BX_ASSERT(srgbCreateInfo.format == TextureFormat::RGBA8_UNORM_SRGB, "Srgb texture must be in RGBA8_UNORM_SRGB.");
    BX_ASSERT(linearCreateInfo.format == TextureFormat::RGBA8_UNORM, "Linear texture must be in RGBA8_UNORM.");
    BX_ASSERT(srgbCreateInfo.size == linearCreateInfo.size, "Srgb and linear texture must match in size.");

    width = linearCreateInfo.size.width;
    height = linearCreateInfo.size.height;

    srgbTextureView = Graphics::CreateTextureView(srgbTexture);
    linearTextureView = Graphics::CreateTextureView(linearTexture);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Srgb to Linear BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(SrgbToLinearPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::TextureView(srgbTextureView)),
        BindGroupEntry(1, BindingResource::TextureView(linearTextureView))
    };
    bindGroup = Graphics::CreateBindGroup(createInfo);
}

SrgbToLinearPass::~SrgbToLinearPass()
{
    Graphics::DestroyBindGroup(bindGroup);
    Graphics::DestroyTextureView(srgbTextureView);
    Graphics::DestroyTextureView(linearTextureView);
}

void SrgbToLinearPass::Dispatch()
{
    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Srgb to Linear";

    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(SrgbToLinearPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);
}

void SrgbToLinearPass::ClearPipelineCache()
{
    SrgbToLinearPipeline::Clear();
}