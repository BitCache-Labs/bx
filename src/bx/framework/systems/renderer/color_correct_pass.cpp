#include "bx/framework/systems/renderer/color_correct_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

struct ColorCorrectConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct ColorCorrectPipeline : public LazyInit<ColorCorrectPipeline, ComputePipelineHandle>
{
    ColorCorrectPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Color Correct Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/color_correct/color_correct.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ_WRITE, TextureFormat::RGBA32_FLOAT)),
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Color Correct Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};

template<>
std::unique_ptr<ColorCorrectPipeline> LazyInit<ColorCorrectPipeline, ComputePipelineHandle>::cache = nullptr;

ColorCorrectPass::ColorCorrectPass(TextureHandle colorTarget)
{
    const TextureCreateInfo& colorTargetCreateInfo = Graphics::GetTextureCreateInfo(colorTarget);
    width = colorTargetCreateInfo.size.width;
    height = colorTargetCreateInfo.size.height;

    colorTargetView = Graphics::CreateTextureView(colorTarget);

    ColorCorrectConstants constants{};
    constants.width = width;
    constants.height = height;

    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "Color Correct Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(ColorCorrectConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBufferCreateInfo.data = &constants;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Color Correct BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(ColorCorrectPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(constantBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(colorTargetView)),
    };
    bindGroup = Graphics::CreateBindGroup(createInfo);
}

ColorCorrectPass::~ColorCorrectPass()
{
    Graphics::DestroyBuffer(constantBuffer);
    Graphics::DestroyTextureView(colorTargetView);

    Graphics::DestroyBindGroup(bindGroup);
}

void ColorCorrectPass::Dispatch()
{
    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Color Correct";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(ColorCorrectPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);
}

void ColorCorrectPass::ClearPipelineCache()
{
    ColorCorrectPipeline::Clear();
}