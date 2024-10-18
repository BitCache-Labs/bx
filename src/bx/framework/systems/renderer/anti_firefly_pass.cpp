#include "bx/framework/systems/renderer/anti_firefly_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

struct AntiFireflyConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct AntiFireflyPipeline : public LazyInit<AntiFireflyPipeline, ComputePipelineHandle>
{
    AntiFireflyPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "AntiFirefly Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/anti_firefly/anti_firefly.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler()),
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "AntiFirefly Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};

template<>
std::unique_ptr<AntiFireflyPipeline> LazyInit<AntiFireflyPipeline, ComputePipelineHandle>::cache = nullptr;

AntiFireflyPass::AntiFireflyPass(u32 width, u32 height)
    : width(width), height(height)
{
    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "AntiFirefly Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(AntiFireflyConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);

    TextureCreateInfo outputTargetCreateInfo{};
    outputTargetCreateInfo.name = "AntiFirefly Output Target";
    outputTargetCreateInfo.size = Extend3D(width, height, 1);
    outputTargetCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    outputTargetCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::COPY_SRC;
    resolvedColorTarget = Graphics::CreateTexture(outputTargetCreateInfo);
    resolvedColorTargetView = Graphics::CreateTextureView(resolvedColorTarget);

    SamplerCreateInfo linearClampCreateInfo{};
    linearClampCreateInfo.name = "AntiFirefly Linear Clamp Sampler";
    linearClampCreateInfo.addressModeU = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.addressModeV = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.minFilter = FilterMode::LINEAR;
    linearClampCreateInfo.magFilter = FilterMode::LINEAR;
    linearClampSampler = Graphics::CreateSampler(linearClampCreateInfo);
}

AntiFireflyPass::~AntiFireflyPass()
{
    Graphics::DestroyBuffer(constantBuffer);

    Graphics::DestroyTextureView(resolvedColorTargetView);
    Graphics::DestroyTexture(resolvedColorTarget);

    Graphics::DestroySampler(linearClampSampler);
}

TextureHandle AntiFireflyPass::GetResolvedColorTarget() const
{
    return resolvedColorTarget;
}

void AntiFireflyPass::Dispatch(TextureHandle colorTarget)
{
    AntiFireflyConstants constants{};
    constants.width = width;
    constants.height = height;
    Graphics::WriteBuffer(constantBuffer, 0, &constants, sizeof(AntiFireflyConstants));

    Graphics::BuildTextureMips(colorTarget);

    TextureViewHandle colorTargetView = Graphics::CreateTextureView(colorTarget);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "AntiFirefly BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(AntiFireflyPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(constantBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(colorTargetView)),
        BindGroupEntry(2, BindingResource::TextureView(resolvedColorTargetView)),
        BindGroupEntry(3, BindingResource::Sampler(linearClampSampler)),
    };
    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Anti Firefly";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(AntiFireflyPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::DestroyTextureView(colorTargetView);
    Graphics::DestroyBindGroup(bindGroup);
}

void AntiFireflyPass::ClearPipelineCache()
{
    AntiFireflyPipeline::Clear();
}