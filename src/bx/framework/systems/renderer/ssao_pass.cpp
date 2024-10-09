#include "bx/framework/systems/renderer/ssao_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

struct SsaoConstants
{
    u32 width;
    u32 height;
    u32 sampleCount;
    b32 reducedBias;
    u32 seed;
    u32 _PADDING0;
    u32 _PADDING1;
    u32 _PADDING2;
};

struct SsaoPipeline : public LazyInit<SsaoPipeline, ComputePipelineHandle>
{
    SsaoPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "SSAO Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/ssao/ssao.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ_WRITE, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "SSAO Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};

template<>
std::unique_ptr<SsaoPipeline> LazyInit<SsaoPipeline, ComputePipelineHandle>::cache = nullptr;

SsaoPass::SsaoPass(u32 width, u32 height)
    : width(width), height(height)
{
    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "Ssao Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(SsaoConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);
}

SsaoPass::~SsaoPass()
{
    Graphics::DestroyBuffer(constantBuffer);
}

void SsaoPass::Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle ambientEmissiveBaseColorView)
{
    SsaoConstants constants{};
    constants.width = width;
    constants.height = height;
    constants.sampleCount = sampleCount;
    constants.reducedBias = reducedBias;
    constants.seed = seed;
    Graphics::WriteBuffer(constantBuffer, 0, &constants, sizeof(SsaoConstants));

    TextureViewHandle colorTargetView = Graphics::CreateTextureView(colorTarget);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Ssao BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(SsaoPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(constantBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(colorTargetView)),
        BindGroupEntry(2, BindingResource::TextureView(gbufferView)),
        BindGroupEntry(3, BindingResource::TextureView(ambientEmissiveBaseColorView)),
    };
    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Ssao";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(SsaoPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::DestroyTextureView(colorTargetView);
    Graphics::DestroyBindGroup(bindGroup);
}

void SsaoPass::ClearPipelineCache()
{
    SsaoPipeline::Clear();
}