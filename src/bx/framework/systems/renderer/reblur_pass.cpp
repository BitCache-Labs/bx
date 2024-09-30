#include "bx/framework/systems/renderer/reblur_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/engine/core/file.hpp"

struct PreBlurConstants
{
    u32 width;
    u32 height;
    u32 seed;
    u32 _PADDING0;
};

struct TemporalAccumConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct HistoryFixConstants
{
    u32 width;
    u32 height;
    b32 antiFirefly;
    u32 _PADDING0;
};

struct BlurConstants
{
    u32 width;
    u32 height;
    u32 seed;
    u32 _PADDING0;
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
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // gbuffer
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outImage
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
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // neGbufferHistory
                BindGroupLayoutEntry(7, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // velocity
                BindGroupLayoutEntry(8, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outImage
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

struct HistoryFixPipeline : public LazyInit<HistoryFixPipeline, ComputePipelineHandle>
{
    HistoryFixPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Reblur History Fix Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/reblur/history_fix.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                                                 // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),                         // inImage
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),                                                               // history
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),                         // outHistory
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),                        // outImage
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Reblur History Fix Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<HistoryFixPipeline> LazyInit<HistoryFixPipeline, ComputePipelineHandle>::cache = nullptr;

struct BlurPipeline : public LazyInit<BlurPipeline, ComputePipelineHandle>
{
    BlurPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Reblur Blur Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/reblur/blur.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                             // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // inImage
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // gbuffer
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // outHistory
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outImage
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Reblur Blur Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<BlurPipeline> LazyInit<BlurPipeline, ComputePipelineHandle>::cache = nullptr;

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

    BufferCreateInfo historyFixConstantsCreateInfo{};
    historyFixConstantsCreateInfo.name = "Reblur History Fix Constants Buffer";
    historyFixConstantsCreateInfo.size = sizeof(HistoryFixConstants);
    historyFixConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    historyFixConstantsBuffer = Graphics::CreateBuffer(historyFixConstantsCreateInfo);

    BufferCreateInfo blurConstantsCreateInfo{};
    blurConstantsCreateInfo.name = "Reblur Blur Constants Buffer";
    blurConstantsCreateInfo.size = sizeof(BlurConstants);
    blurConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    blurConstantsBuffer = Graphics::CreateBuffer(blurConstantsCreateInfo);

    TextureCreateInfo tmpIlluminationCreateInfo{};
    tmpIlluminationCreateInfo.name = "Reblur Temp Illumination Texture";
    tmpIlluminationCreateInfo.size = Extend3D(width, height, 1);
    tmpIlluminationCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    tmpIlluminationCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::COPY_SRC;
    tmpIlluminationTexture = Graphics::CreateTexture(tmpIlluminationCreateInfo);
    tmpIlluminationTextureView = Graphics::CreateTextureView(tmpIlluminationTexture);

    TextureCreateInfo preBlurCreateInfo{};
    preBlurCreateInfo.name = "Reblur Pre Blur Texture";
    preBlurCreateInfo.size = Extend3D(width, height, 1);
    BX_ASSERT(Math::MipLevelsFromDims(width, height) >= 7, "Nert pass too small.");
    preBlurCreateInfo.mipLevelCount = 7;
    preBlurCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    preBlurCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::COPY_SRC | TextureUsageFlags::COPY_DST | TextureUsageFlags::TEXTURE_BINDING;
    preBlurTexture = Graphics::CreateTexture(preBlurCreateInfo);
    preBlurTextureView = Graphics::CreateTextureView(preBlurTexture);

    TextureCreateInfo historyCreateInfo{};
    historyCreateInfo.size = Extend3D(width, height, 1);
    historyCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    historyCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING;
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
    Graphics::DestroyBuffer(temporalAccumConstantsBuffer);
    Graphics::DestroyBuffer(historyFixConstantsBuffer);
    Graphics::DestroyBuffer(blurConstantsBuffer);

    Graphics::DestroyTextureView(preBlurTextureView);
    Graphics::DestroyTexture(preBlurTexture);
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
        BindGroupEntry(2, BindingResource::TextureView(dispatchInfo.gbufferView)),
        BindGroupEntry(3, BindingResource::TextureView(preBlurTextureView))
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
        BindGroupEntry(1, BindingResource::TextureView(preBlurTextureView)),
        BindGroupEntry(2, BindingResource::TextureView(historyTextureView[frameIdx % 2 == 0])),
        BindGroupEntry(3, BindingResource::TextureView(historyTextureView[frameIdx % 2 != 0])),
        BindGroupEntry(4, BindingResource::TextureView(dispatchInfo.gbufferView)),
        BindGroupEntry(5, BindingResource::TextureView(dispatchInfo.gbufferHistoryView)),
        BindGroupEntry(6, BindingResource::TextureView(dispatchInfo.neGbufferHistoryView)),
        BindGroupEntry(7, BindingResource::TextureView(dispatchInfo.velocityView)),
        BindGroupEntry(8, BindingResource::TextureView(tmpIlluminationTextureView)),
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);

    return bindGroup;
}

BindGroupHandle ReblurPass::CreateHistoryFixBindGroup(const ReblurDispatchInfo& dispatchInfo) const
{
    TextureViewHandle unresolvedIlluminationView = Graphics::CreateTextureView(dispatchInfo.unresolvedIllumination);

    TextureViewHandle historyView = Graphics::CreateTextureView(historyTexture[frameIdx % 2 == 0]);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reblur History Fix Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(HistoryFixPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(historyFixConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(tmpIlluminationTextureView)),
        BindGroupEntry(2, BindingResource::TextureView(preBlurTextureView)),
        BindGroupEntry(3, BindingResource::TextureView(historyTextureView[frameIdx % 2 != 0])),
        BindGroupEntry(4, BindingResource::TextureView(unresolvedIlluminationView)),
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);
    Graphics::DestroyTextureView(historyView);

    return bindGroup;
}

BindGroupHandle ReblurPass::CreateBlurBindGroup(const ReblurDispatchInfo& dispatchInfo) const
{
    TextureViewHandle unresolvedIlluminationView = Graphics::CreateTextureView(dispatchInfo.unresolvedIllumination);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reblur Blur Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(BlurPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(preBlurConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(unresolvedIlluminationView)),
        BindGroupEntry(2, BindingResource::TextureView(dispatchInfo.gbufferView)),
        BindGroupEntry(3, BindingResource::TextureView(historyTextureView[frameIdx % 2 != 0])),
        BindGroupEntry(4, BindingResource::TextureView(tmpIlluminationTextureView))
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);

    return bindGroup;
}

void ReblurPass::Dispatch(const ReblurDispatchInfo& dispatchInfo)
{
    BindGroupHandle preBlurBindGroup = CreatePreBlurBindGroup(dispatchInfo);
    BindGroupHandle temporalAccumBindGroup = CreateTemporalAccumBindGroup(dispatchInfo);
    BindGroupHandle historyFixBindGroup = CreateHistoryFixBindGroup(dispatchInfo);
    BindGroupHandle blurBindGroup = CreateBlurBindGroup(dispatchInfo);

    PreBlurConstants preBlurConstants{};
    preBlurConstants.width = width;
    preBlurConstants.height = height;
    preBlurConstants.seed = seed;
    Graphics::WriteBuffer(preBlurConstantsBuffer, 0, &preBlurConstants);

    TemporalAccumConstants temporalAccumConstants{};
    temporalAccumConstants.width = width;
    temporalAccumConstants.height = height;
    Graphics::WriteBuffer(temporalAccumConstantsBuffer, 0, &temporalAccumConstants);

    HistoryFixConstants historyFixConstants{};
    historyFixConstants.width = width;
    historyFixConstants.height = height;
    historyFixConstants.antiFirefly = antiFirefly;
    Graphics::WriteBuffer(historyFixConstantsBuffer, 0, &historyFixConstants);

    BlurConstants blurConstants{};
    blurConstants.width = width;
    blurConstants.height = height;
    blurConstants.seed = seed;
    Graphics::WriteBuffer(blurConstantsBuffer, 0, &blurConstants);

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
    
    Graphics::BuildTextureMips(preBlurTexture);
    
    computePassDescriptor.name = "Reblur History Fix";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(HistoryFixPipeline::Get());
        Graphics::SetBindGroup(0, historyFixBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);
    
    computePassDescriptor.name = "Reblur Blur";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(BlurPipeline::Get());
        Graphics::SetBindGroup(0, blurBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    // TODO: remove
    Graphics::CopyTexture(tmpIlluminationTexture, dispatchInfo.unresolvedIllumination);
    //Graphics::CopyTexture(preBlurTexture, dispatchInfo.unresolvedIllumination);

    Graphics::DestroyBindGroup(preBlurBindGroup);
    Graphics::DestroyBindGroup(temporalAccumBindGroup);
    Graphics::DestroyBindGroup(historyFixBindGroup);
    Graphics::DestroyBindGroup(blurBindGroup);

    frameIdx++;
}

void ReblurPass::ClearPipelineCache()
{
    PreBlurPipeline::Clear();
}