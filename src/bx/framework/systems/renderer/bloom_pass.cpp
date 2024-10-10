#include "bx/framework/systems/renderer/bloom_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

struct DownsampleConstants
{
    u32 width;
    u32 height;
    f32 fogStart;
    f32 fogEnd;
};

struct DownsamplePipeline : public LazyInit<DownsamplePipeline, ComputePipelineHandle>
{
    DownsamplePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Downsample Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/bloom/downsample.comp.shader")));
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
        pipelineCreateInfo.name = "Downsample Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};

template<>
std::unique_ptr<DownsamplePipeline> LazyInit<DownsamplePipeline, ComputePipelineHandle>::cache = nullptr;

BloomPass::BloomPass(u32 width, u32 height)
    : width(width), height(height)
{
    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "Bloom Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(BloomConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);

    TextureCreateInfo mippedColorTargetCreateInfo{};
    mippedColorTargetCreateInfo.name = "Bloom Pass Mipped Color Target Texture";
    mippedColorTargetCreateInfo.size = Extend3D(width, height, 1);
    mippedColorTargetCreateInfo.mipLevelCount = Math::MipLevelsFromDims(width, height);
    mippedColorTargetCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    mippedColorTargetCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::TEXTURE_BINDING;
    mippedColorTarget = Graphics::CreateTexture(mippedColorTargetCreateInfo);
}

BloomPass::~BloomPass()
{
    Graphics::DestroyBuffer(constantBuffer);
    Graphics::DestroyTexture(mippedColorTarget);
}

void BloomPass::Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle ambientEmissiveBaseColorView)
{
    BloomConstants constants{};
    constants.width = width;
    constants.height = height;
    constants.fogStart = 5.0;
    constants.fogEnd = 30.0;
    Graphics::WriteBuffer(constantBuffer, 0, &constants, sizeof(BloomConstants));

    TextureViewHandle colorTargetView = Graphics::CreateTextureView(colorTarget);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Bloom BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(BloomPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(constantBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(colorTargetView)),
        BindGroupEntry(2, BindingResource::TextureView(gbufferView)),
        BindGroupEntry(3, BindingResource::TextureView(ambientEmissiveBaseColorView)),
    };
    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Bloom";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(BloomPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::DestroyTextureView(colorTargetView);
    Graphics::DestroyBindGroup(bindGroup);
}

void BloomPass::ClearPipelineCache()
{
    BloomPipeline::Clear();
}