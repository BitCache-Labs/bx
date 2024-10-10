#include "bx/framework/systems/renderer/bloom_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

struct BloomConstants
{
    u32 srcWidth;
    u32 srcHeight;
    u32 dstWidth;
    u32 dstHeight;
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
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),
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
    mipCount = Math::MipLevelsFromDims(width, height);

    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "Bloom Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(BloomConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);

    TextureCreateInfo mippedColorTargetCreateInfo{};
    mippedColorTargetCreateInfo.name = "Bloom Pass Mipped Color Target Texture";
    mippedColorTargetCreateInfo.size = Extend3D(width, height, 1);
    mippedColorTargetCreateInfo.mipLevelCount = mipCount;
    mippedColorTargetCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    mippedColorTargetCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::TEXTURE_BINDING;
    mippedColorTarget = Graphics::CreateTexture(mippedColorTargetCreateInfo);
}

BloomPass::~BloomPass()
{
    Graphics::DestroyBuffer(constantBuffer);
    Graphics::DestroyTexture(mippedColorTarget);
}

void BloomPass::Dispatch(const Camera& camera, TextureHandle colorTarget)
{
    Graphics::CopyTexture(colorTarget, mippedColorTarget);

    f32 mipWidth = static_cast<f32>(width);
    f32 mipHeight = static_cast<f32>(height);
    u32 mipWidthInt = static_cast<u32>(mipWidth);
    u32 mipHeightInt = static_cast<u32>(mipHeight);

    for (u32 i = 0; i < mipCount - 3; i++) // TODO: go all the way?
    {
        BloomConstants constants{};
        constants.srcWidth = mipWidthInt;
        constants.srcHeight = mipHeightInt;

        mipWidth *= 0.5;
        mipHeight *= 0.5;
        mipWidthInt = static_cast<u32>(mipWidth);
        mipHeightInt = static_cast<u32>(mipHeight);
        
        constants.dstWidth = mipWidthInt;
        constants.dstHeight = mipHeightInt;
        Graphics::WriteBufferImmediate(constantBuffer, 0, &constants, sizeof(BloomConstants));

        TextureViewCreateInfo srcViewCreateInfo{};
        srcViewCreateInfo.texture = mippedColorTarget;
        srcViewCreateInfo.baseMipLevel = i;
        TextureViewHandle srcView = Graphics::CreateTextureView(srcViewCreateInfo);

        TextureViewCreateInfo dstViewCreateInfo{};
        dstViewCreateInfo.texture = mippedColorTarget;
        dstViewCreateInfo.baseMipLevel = i + 1;
        TextureViewHandle dstView = Graphics::CreateTextureView(dstViewCreateInfo);

        BindGroupCreateInfo createInfo{};
        createInfo.name = "Bloom Downsample BindGroup";
        createInfo.layout = Graphics::GetBindGroupLayout(DownsamplePipeline::Get(), 0);
        createInfo.entries = {
            BindGroupEntry(0, BindingResource::Buffer(constantBuffer)),
            BindGroupEntry(1, BindingResource::TextureView(srcView)),
            BindGroupEntry(2, BindingResource::TextureView(dstView)),
        };
        BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

        ComputePassDescriptor computePassDescriptor{};
        computePassDescriptor.name = "Bloom Downsample";
        ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(DownsamplePipeline::Get());
            Graphics::SetBindGroup(0, bindGroup);
            Graphics::DispatchWorkgroups(Math::DivCeil(mipWidthInt, 16), Math::DivCeil(mipHeightInt, 16), 1);
        }
        Graphics::EndComputePass(computePass);

        Graphics::DestroyBindGroup(bindGroup);
    }
}

void BloomPass::ClearPipelineCache()
{
    DownsamplePipeline::Clear();
}