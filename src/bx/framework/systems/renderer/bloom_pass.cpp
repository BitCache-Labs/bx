#include "bx/framework/systems/renderer/bloom_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

namespace BloomPipelines
{
    struct BloomConstants
    {
        u32 srcWidth;
        u32 srcHeight;
        u32 dstWidth;
        u32 dstHeight;
    };

    struct ResolveConstants
    {
        u32 width;
        u32 height;
        f32 intensity;
        u32 _PADDING0;
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
                    BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
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
    
    struct UpsamplePipeline : public LazyInit<UpsamplePipeline, ComputePipelineHandle>
    {
        UpsamplePipeline()
        {
            ShaderCreateInfo shaderCreateInfo{};
            shaderCreateInfo.name = "Upsample Shader";
            shaderCreateInfo.shaderType = ShaderType::COMPUTE;
            shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/bloom/upsample.comp.shader")));
            ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

            PipelineLayoutDescriptor pipelineLayoutDescriptor{};
            pipelineLayoutDescriptor.bindGroupLayouts = {
                BindGroupLayoutDescriptor(0, {
                    BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                    BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
                    BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),
                })
            };

            ComputePipelineCreateInfo pipelineCreateInfo{};
            pipelineCreateInfo.name = "Upsample Pipeline";
            pipelineCreateInfo.layout = pipelineLayoutDescriptor;
            pipelineCreateInfo.shader = shader;
            data = Graphics::CreateComputePipeline(pipelineCreateInfo);

            Graphics::DestroyShader(shader);
        }
    };

    struct ResolvePipeline : public LazyInit<ResolvePipeline, ComputePipelineHandle>
    {
        ResolvePipeline()
        {
            ShaderCreateInfo shaderCreateInfo{};
            shaderCreateInfo.name = "Resolve Shader";
            shaderCreateInfo.shaderType = ShaderType::COMPUTE;
            shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/bloom/resolve.comp.shader")));
            ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

            PipelineLayoutDescriptor pipelineLayoutDescriptor{};
            pipelineLayoutDescriptor.bindGroupLayouts = {
                BindGroupLayoutDescriptor(0, {
                    BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                    BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                    BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ_WRITE, TextureFormat::RGBA32_FLOAT)),
                })
            };

            ComputePipelineCreateInfo pipelineCreateInfo{};
            pipelineCreateInfo.name = "Resolve Pipeline";
            pipelineCreateInfo.layout = pipelineLayoutDescriptor;
            pipelineCreateInfo.shader = shader;
            data = Graphics::CreateComputePipeline(pipelineCreateInfo);

            Graphics::DestroyShader(shader);
        }
    };
}

using namespace BloomPipelines;

template<>
std::unique_ptr<DownsamplePipeline> LazyInit<DownsamplePipeline, ComputePipelineHandle>::cache = nullptr;
template<>
std::unique_ptr<UpsamplePipeline> LazyInit<UpsamplePipeline, ComputePipelineHandle>::cache = nullptr;
template<>
std::unique_ptr<ResolvePipeline> LazyInit<ResolvePipeline, ComputePipelineHandle>::cache = nullptr;

BloomPass::BloomPass(u32 width, u32 height)
    : width(width), height(height)
{
    mipCount = Math::MipLevelsFromDims(width, height);

    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "Bloom Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(BloomConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);

    BufferCreateInfo resolveConstantBufferCreateInfo{};
    resolveConstantBufferCreateInfo.name = "Bloom Pass Resolve Constant Buffer";
    resolveConstantBufferCreateInfo.size = sizeof(ResolveConstants);
    resolveConstantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    resolveConstantBuffer = Graphics::CreateBuffer(resolveConstantBufferCreateInfo);

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
    Graphics::DestroyBuffer(resolveConstantBuffer);
    Graphics::DestroyTexture(mippedColorTarget);
}

void BloomPass::Dispatch(const Camera& camera, TextureHandle colorTarget)
{
    Graphics::CopyTexture(colorTarget, mippedColorTarget);

    f32 mipWidth = static_cast<f32>(width);
    f32 mipHeight = static_cast<f32>(height);
    u32 mipWidthInt = static_cast<u32>(mipWidth);
    u32 mipHeightInt = static_cast<u32>(mipHeight);

    // TODO: use sampler with clamp instead of repeat

    for (u32 i = 0; i < mipCount; i++)
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
        srcViewCreateInfo.mipLevelCount = Optional<u32>::Some(1);
        TextureViewHandle srcView = Graphics::CreateTextureView(srcViewCreateInfo);

        TextureViewCreateInfo dstViewCreateInfo{};
        dstViewCreateInfo.texture = mippedColorTarget;
        dstViewCreateInfo.baseMipLevel = i + 1;
        dstViewCreateInfo.mipLevelCount = Optional<u32>::Some(1);
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

        Graphics::DestroyTextureView(srcView);
        Graphics::DestroyTextureView(dstView);
        Graphics::DestroyBindGroup(bindGroup);
    }

    for (i32 i = mipCount; i >= 1; i--)
    {
        BloomConstants constants{};
        constants.srcWidth = mipWidthInt;
        constants.srcHeight = mipHeightInt;

        mipWidth *= 2.0;
        mipHeight *= 2.0;
        mipWidthInt = static_cast<u32>(mipWidth);
        mipHeightInt = static_cast<u32>(mipHeight);

        constants.dstWidth = mipWidthInt;
        constants.dstHeight = mipHeightInt;
        Graphics::WriteBufferImmediate(constantBuffer, 0, &constants, sizeof(BloomConstants));

        TextureViewCreateInfo srcViewCreateInfo{};
        srcViewCreateInfo.texture = mippedColorTarget;
        srcViewCreateInfo.baseMipLevel = i;
        srcViewCreateInfo.mipLevelCount = Optional<u32>::Some(1);
        TextureViewHandle srcView = Graphics::CreateTextureView(srcViewCreateInfo);

        TextureViewCreateInfo dstViewCreateInfo{};
        dstViewCreateInfo.texture = mippedColorTarget;
        dstViewCreateInfo.baseMipLevel = i - 1;
        dstViewCreateInfo.mipLevelCount = Optional<u32>::Some(1);
        TextureViewHandle dstView = Graphics::CreateTextureView(dstViewCreateInfo);

        BindGroupCreateInfo createInfo{};
        createInfo.name = "Bloom Upsample BindGroup";
        createInfo.layout = Graphics::GetBindGroupLayout(UpsamplePipeline::Get(), 0);
        createInfo.entries = {
            BindGroupEntry(0, BindingResource::Buffer(constantBuffer)),
            BindGroupEntry(1, BindingResource::TextureView(srcView)),
            BindGroupEntry(2, BindingResource::TextureView(dstView)),
        };
        BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

        ComputePassDescriptor computePassDescriptor{};
        computePassDescriptor.name = "Bloom Upsample";
        ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(UpsamplePipeline::Get());
            Graphics::SetBindGroup(0, bindGroup);
            Graphics::DispatchWorkgroups(Math::DivCeil(mipWidthInt, 16), Math::DivCeil(mipHeightInt, 16), 1);
        }
        Graphics::EndComputePass(computePass);

        Graphics::DestroyTextureView(srcView);
        Graphics::DestroyTextureView(dstView);
        Graphics::DestroyBindGroup(bindGroup);
    }

    ResolveConstants resolveConstants{};
    resolveConstants.width = width;
    resolveConstants.height = height;
    resolveConstants.intensity = intensity;
    Graphics::WriteBuffer(resolveConstantBuffer, 0, &resolveConstants);

    TextureViewHandle srcView = Graphics::CreateTextureView(mippedColorTarget);
    TextureViewHandle dstView = Graphics::CreateTextureView(colorTarget);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Bloom Resolve BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(ResolvePipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(resolveConstantBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(srcView)),
        BindGroupEntry(2, BindingResource::TextureView(dstView)),
    };
    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Bloom Resolve";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(ResolvePipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::DestroyTextureView(srcView);
    Graphics::DestroyTextureView(dstView);
    Graphics::DestroyBindGroup(bindGroup);
}

void BloomPass::ClearPipelineCache()
{
    DownsamplePipeline::Clear();
}