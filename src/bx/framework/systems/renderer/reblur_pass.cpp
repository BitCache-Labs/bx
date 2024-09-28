#include "bx/framework/systems/renderer/reblur_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/engine/core/file.hpp"

struct PreBlurConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct TemporalAccumConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct PreBlurPipeline : public LazyInit<PreBlurPipeline, ComputePipelineHandle>
{
    PreBlurPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Reblur Pre Blur Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/reblur/pre_blur.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                             // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // inImage
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outImage
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Reblur Pre Blur Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<PreBlurPipeline> LazyInit<PreBlurPipeline, ComputePipelineHandle>::cache = nullptr;

struct TemporalAccumPipeline : public LazyInit<TemporalAccumPipeline, ComputePipelineHandle>
{
    TemporalAccumPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Reblur Temporal Accum Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/reblur/temporal_accum.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                             // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // inImage
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // history
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outHistory
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // gbuffer
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // gbufferHistory
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // velocity
                BindGroupLayoutEntry(7, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outImage
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Reblur Temporal Accum Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<TemporalAccumPipeline> LazyInit<TemporalAccumPipeline, ComputePipelineHandle>::cache = nullptr;

ReblurPass::ReblurPass(u32 width, u32 height)
    : width(width), height(height), frameIdx(0)
{
    BufferCreateInfo preBlurConstantsCreateInfo{};
    preBlurConstantsCreateInfo.name = "Reblur Pre Blur Constants Buffer";
    preBlurConstantsCreateInfo.size = sizeof(PreBlurConstants);
    preBlurConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    preBlurConstantsBuffer = Graphics::CreateBuffer(preBlurConstantsCreateInfo);

    BufferCreateInfo temporalAccumConstantsCreateInfo{};
    temporalAccumConstantsCreateInfo.name = "Reblur Temporal Accum Constants Buffer";
    temporalAccumConstantsCreateInfo.size = sizeof(TemporalAccumConstants);
    temporalAccumConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    temporalAccumConstantsBuffer = Graphics::CreateBuffer(temporalAccumConstantsCreateInfo);

    TextureCreateInfo tmpIlluminationCreateInfo{};
    tmpIlluminationCreateInfo.name = "Reblur Temp Illumination Texture";
    tmpIlluminationCreateInfo.size = Extend3D(width, height, 1);
    tmpIlluminationCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    tmpIlluminationCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::COPY_SRC;
    tmpIlluminationTexture = Graphics::CreateTexture(tmpIlluminationCreateInfo);
    tmpIlluminationTextureView = Graphics::CreateTextureView(tmpIlluminationTexture);

    TextureCreateInfo historyCreateInfo{};
    historyCreateInfo.size = Extend3D(width, height, 1);
    historyCreateInfo.mipLevelCount = Math::MipLevelsFromDims(width, height);
    historyCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    historyCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::COPY_SRC | TextureUsageFlags::COPY_DST | TextureUsageFlags::TEXTURE_BINDING;
    for (u32 i = 0; i < 2; i++)
    {
        historyCreateInfo.name = Log::Format("Reblur History {} Texture", i);
        historyTexture[i] = Graphics::CreateTexture(historyCreateInfo);
        historyTextureView[i] = Graphics::CreateTextureView(historyTexture[i]);
    }
}

ReblurPass::~ReblurPass()
{
    Graphics::DestroyBuffer(preBlurConstantsBuffer);

    Graphics::DestroyTextureView(tmpIlluminationTextureView);
    Graphics::DestroyTexture(tmpIlluminationTexture);

    for (u32 i = 0; i < 2; i++)
    {
        Graphics::DestroyTextureView(historyTextureView[i]);
        Graphics::DestroyTexture(historyTexture[i]);
    }
}

BindGroupHandle ReblurPass::CreatePreBlurBindGroup(const ReblurDispatchInfo& dispatchInfo) const
{
    TextureViewHandle unresolvedIlluminationView = Graphics::CreateTextureView(dispatchInfo.unresolvedIllumination);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reblur Pre Blur Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(PreBlurPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(preBlurConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(unresolvedIlluminationView)),
        BindGroupEntry(2, BindingResource::TextureView(tmpIlluminationTextureView))
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);

    return bindGroup;
}

BindGroupHandle ReblurPass::CreateTemporalAccumBindGroup(const ReblurDispatchInfo& dispatchInfo) const
{
    TextureViewHandle unresolvedIlluminationView = Graphics::CreateTextureView(dispatchInfo.unresolvedIllumination);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reblur Temporal Accum Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(TemporalAccumPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(temporalAccumConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(tmpIlluminationTextureView)),
        BindGroupEntry(2, BindingResource::TextureView(historyTextureView[frameIdx % 2 == 0])),
        BindGroupEntry(3, BindingResource::TextureView(historyTextureView[frameIdx % 2 != 0])),
        BindGroupEntry(4, BindingResource::TextureView(dispatchInfo.gbufferView)),
        BindGroupEntry(5, BindingResource::TextureView(dispatchInfo.gbufferHistoryView)),
        BindGroupEntry(6, BindingResource::TextureView(dispatchInfo.velocityView)),
        BindGroupEntry(7, BindingResource::TextureView(unresolvedIlluminationView)),
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);

    return bindGroup;
}

void ReblurPass::Dispatch(const ReblurDispatchInfo& dispatchInfo)
{
    BindGroupHandle preBlurBindGroup = CreatePreBlurBindGroup(dispatchInfo);
    BindGroupHandle temporalAccumBindGroup = CreateTemporalAccumBindGroup(dispatchInfo);

    PreBlurConstants preBlurConstants{};
    preBlurConstants.width = width;
    preBlurConstants.height = height;
    Graphics::WriteBuffer(preBlurConstantsBuffer, 0, &preBlurConstants);

    TemporalAccumConstants temporalAccumConstants{};
    temporalAccumConstants.width = width;
    temporalAccumConstants.height = height;
    Graphics::WriteBuffer(temporalAccumConstantsBuffer, 0, &temporalAccumConstants);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Reblur Pre Blur";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(PreBlurPipeline::Get());
        Graphics::SetBindGroup(0, preBlurBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    computePassDescriptor.name = "Reblur Temporal Accum";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(TemporalAccumPipeline::Get());
        Graphics::SetBindGroup(0, temporalAccumBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::BuildTextureMips(historyTexture[frameIdx % 2 != 0]); // TODO: should this be history out or not?

    //Graphics::CopyTexture(tmpIlluminationTexture, dispatchInfo.unresolvedIllumination);

    Graphics::DestroyBindGroup(preBlurBindGroup);
    Graphics::DestroyBindGroup(temporalAccumBindGroup);

    frameIdx++;
}

void ReblurPass::ClearPipelineCache()
{
    PreBlurPipeline::Clear();
}