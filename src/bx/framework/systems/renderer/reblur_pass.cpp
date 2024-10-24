#include "bx/framework/systems/renderer/reblur_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/engine/core/file.hpp"

struct TemporalAccumConstants
{
    u32 globalWidth;
    u32 globalHeight;
    u32 width;
    u32 height;
    b32 antiFirefly;
    u32 _PADDING0;
    u32 _PADDING1;
    u32 _PADDING2;
};

struct ATrousConstants
{
    u32 globalWidth;
    u32 globalHeight;
    u32 width;
    u32 height;
    u32 stepSize;
    b32 writeHistory;
    u32 _PADDING0;
    u32 _PADDING1;
};

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
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                                 // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),         // inImage
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),         // history
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),        // outHistory
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),         // gbuffer
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),         // gbufferHistory
                BindGroupLayoutEntry(7, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),          // velocity
                BindGroupLayoutEntry(8, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),   // variance
                BindGroupLayoutEntry(9, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),   // outVariance
                BindGroupLayoutEntry(10, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),        // outImage
                BindGroupLayoutEntry(11, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler()),                                                                      // nearestClampSampler
                BindGroupLayoutEntry(12, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler()),                                                                      // linearClampSampler
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

struct ATrousPipeline : public LazyInit<ATrousPipeline, ComputePipelineHandle>
{
    ATrousPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Reblur A-Trous Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/reblur/a_trous.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                             // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // inImage
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // gbuffer
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // variance
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outImage
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ_WRITE, TextureFormat::RGBA32_FLOAT)),    // outHistory
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),    // depth
                BindGroupLayoutEntry(7, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler()),    // depth
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Reblur A-Trous Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<ATrousPipeline> LazyInit<ATrousPipeline, ComputePipelineHandle>::cache = nullptr;

ReblurPass::ReblurPass(u32 width, u32 height, u32 lightingWidth, u32 lightingHeight)
    : width(width), height(height), lightingWidth(lightingWidth), lightingHeight(lightingHeight), frameIdx(0)
{
    SamplerCreateInfo nearestClampCreateInfo{};
    nearestClampCreateInfo.name = "Reblur Nearest Clamp Sampler";
    nearestClampCreateInfo.addressModeU = SamplerAddressMode::CLAMP_TO_EDGE;
    nearestClampCreateInfo.addressModeV = SamplerAddressMode::CLAMP_TO_EDGE;
    nearestClampCreateInfo.addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
    nearestClampCreateInfo.minFilter = FilterMode::NEAREST;
    nearestClampCreateInfo.magFilter = FilterMode::NEAREST;
    nearestClampSampler = Graphics::CreateSampler(nearestClampCreateInfo);

    SamplerCreateInfo linearClampCreateInfo{};
    linearClampCreateInfo.name = "Reblur Linear Clamp Sampler";
    linearClampCreateInfo.addressModeU = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.addressModeV = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.minFilter = FilterMode::LINEAR;
    linearClampCreateInfo.magFilter = FilterMode::LINEAR;
    linearClampSampler = Graphics::CreateSampler(linearClampCreateInfo);

    BufferCreateInfo temporalAccumConstantsCreateInfo{};
    temporalAccumConstantsCreateInfo.name = "Reblur Temporal Accum Constants Buffer";
    temporalAccumConstantsCreateInfo.size = sizeof(TemporalAccumConstants);
    temporalAccumConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    temporalAccumConstantsBuffer = Graphics::CreateBuffer(temporalAccumConstantsCreateInfo);

    BufferCreateInfo preBlurConstantsCreateInfo{};
    preBlurConstantsCreateInfo.name = "Reblur Pre Blur Constants Buffer";
    preBlurConstantsCreateInfo.size = sizeof(ATrousConstants);
    preBlurConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    aTrousConstantsBuffer = Graphics::CreateBuffer(preBlurConstantsCreateInfo);

    TextureCreateInfo tmpIlluminationCreateInfo{};
    tmpIlluminationCreateInfo.name = "Reblur Temp Illumination Texture";
    tmpIlluminationCreateInfo.size = Extend3D(lightingWidth, lightingHeight, 1);
    tmpIlluminationCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    tmpIlluminationCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::COPY_SRC;
    tmpIlluminationTexture = Graphics::CreateTexture(tmpIlluminationCreateInfo);
    tmpIlluminationTextureView = Graphics::CreateTextureView(tmpIlluminationTexture);

    TextureCreateInfo historyCreateInfo{};
    historyCreateInfo.size = Extend3D(lightingWidth, lightingHeight, 1);
    historyCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    historyCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::TEXTURE_BINDING;
    for (u32 i = 0; i < 2; i++)
    {
        historyCreateInfo.name = Log::Format("Reblur History {} Texture", i);
        historyTexture[i] = Graphics::CreateTexture(historyCreateInfo);
        historyTextureView[i] = Graphics::CreateTextureView(historyTexture[i]);
    }

    TextureCreateInfo varianceCreateInfo{};
    varianceCreateInfo.size = Extend3D(lightingWidth, lightingHeight, 1);
    varianceCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    varianceCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING;
    for (u32 i = 0; i < 2; i++)
    {
        varianceCreateInfo.name = Log::Format("Reblur Variance {} Texture", i);
        varianceTexture[i] = Graphics::CreateTexture(varianceCreateInfo);
        varianceTextureView[i] = Graphics::CreateTextureView(varianceTexture[i]);
    }

    antiFireflyPass = std::unique_ptr<AntiFireflyPass>(new AntiFireflyPass(lightingWidth, lightingHeight));
}

ReblurPass::~ReblurPass()
{
    Graphics::DestroySampler(nearestClampSampler);
    Graphics::DestroySampler(linearClampSampler);

    Graphics::DestroyBuffer(temporalAccumConstantsBuffer);
    Graphics::DestroyBuffer(aTrousConstantsBuffer);

    Graphics::DestroyTextureView(tmpIlluminationTextureView);
    Graphics::DestroyTexture(tmpIlluminationTexture);
    for (u32 i = 0; i < 2; i++)
    {
        Graphics::DestroyTextureView(historyTextureView[i]);
        Graphics::DestroyTexture(historyTexture[i]);
        Graphics::DestroyTextureView(varianceTextureView[i]);
        Graphics::DestroyTexture(varianceTexture[i]);
    }
}

BindGroupHandle ReblurPass::CreateTemporalAccumBindGroup(const ReblurDispatchInfo& dispatchInfo) const
{
    TextureViewHandle unresolvedIlluminationView = Graphics::CreateTextureView(dispatchInfo.unresolvedIllumination);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reblur Temporal Accum Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(TemporalAccumPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(temporalAccumConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(unresolvedIlluminationView)),
        BindGroupEntry(2, BindingResource::TextureView(historyTextureView[frameIdx % 2 == 0])),
        BindGroupEntry(3, BindingResource::TextureView(historyTextureView[frameIdx % 2 != 0])),
        BindGroupEntry(4, BindingResource::TextureView(dispatchInfo.gbufferView)),
        BindGroupEntry(5, BindingResource::TextureView(dispatchInfo.gbufferHistoryView)),
        BindGroupEntry(7, BindingResource::TextureView(dispatchInfo.reprojectionView)),
        BindGroupEntry(8, BindingResource::TextureView(varianceTextureView[frameIdx % 2 == 0])),
        BindGroupEntry(9, BindingResource::TextureView(varianceTextureView[frameIdx % 2 != 0])),
        BindGroupEntry(10, BindingResource::TextureView(tmpIlluminationTextureView)),
        BindGroupEntry(11, BindingResource::Sampler(nearestClampSampler)),
        BindGroupEntry(12, BindingResource::Sampler(linearClampSampler)),
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);

    return bindGroup;
}


BindGroupHandle ReblurPass::CreateATrousBindGroup(const ReblurDispatchInfo& dispatchInfo, bool pingPong) const
{
    TextureViewHandle unresolvedIlluminationView = Graphics::CreateTextureView(dispatchInfo.unresolvedIllumination);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reblur A-Trous Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(ATrousPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(aTrousConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(pingPong ? tmpIlluminationTextureView : unresolvedIlluminationView)),
        BindGroupEntry(2, BindingResource::TextureView(dispatchInfo.gbufferView)),
        BindGroupEntry(3, BindingResource::TextureView(varianceTextureView[frameIdx % 2 != 0])),
        BindGroupEntry(4, BindingResource::TextureView(pingPong ? unresolvedIlluminationView : tmpIlluminationTextureView)),
        BindGroupEntry(5, BindingResource::TextureView(historyTextureView[frameIdx % 2 != 0])),
        BindGroupEntry(6, BindingResource::TextureView(dispatchInfo.velocityView)),
        BindGroupEntry(7, BindingResource::Sampler(nearestClampSampler)),
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);

    return bindGroup;
}

void ReblurPass::Dispatch(const ReblurDispatchInfo& dispatchInfo)
{
    BindGroupHandle temporalAccumBindGroup = CreateTemporalAccumBindGroup(dispatchInfo);

    BindGroupHandle aTrousBindGroup[2];
    for (u32 i = 0; i < 2; i++)
    {
        aTrousBindGroup[i] = CreateATrousBindGroup(dispatchInfo, i % 2 != 0);
    }

    if (antiFirefly)
    {
        antiFireflyPass->Dispatch(dispatchInfo.unresolvedIllumination);
        Graphics::CopyTexture(antiFireflyPass->GetResolvedColorTarget(), dispatchInfo.unresolvedIllumination);
    }

    Graphics::BuildTextureMips(dispatchInfo.unresolvedIllumination);

    TemporalAccumConstants temporalAccumConstants{};
    temporalAccumConstants.globalWidth = width;
    temporalAccumConstants.globalHeight = height;
    temporalAccumConstants.width = lightingWidth;
    temporalAccumConstants.height = lightingHeight;
    temporalAccumConstants.antiFirefly = antiFirefly;
    Graphics::WriteBuffer(temporalAccumConstantsBuffer, 0, &temporalAccumConstants);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Reblur Temporal Accum";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(TemporalAccumPipeline::Get());
        Graphics::SetBindGroup(0, temporalAccumBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(lightingWidth, 16), Math::DivCeil(lightingHeight, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    ATrousConstants aTrousConstants{};
    aTrousConstants.globalWidth = width;
    aTrousConstants.globalHeight = height;
    aTrousConstants.width = lightingWidth;
    aTrousConstants.height = lightingHeight;
    
    for (u32 i = 0; i < spatialFilterSteps; i++)
    {
        aTrousConstants.writeHistory = (i == 0);
        aTrousConstants.stepSize = 1 << i;
        Graphics::WriteBufferImmediate(aTrousConstantsBuffer, 0, &aTrousConstants);
    
        
        computePassDescriptor.name = "Reblur A-Trous";
        computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(ATrousPipeline::Get());
            Graphics::SetBindGroup(0, aTrousBindGroup[i % 2 == 0]);
            Graphics::DispatchWorkgroups(Math::DivCeil(lightingWidth, 16), Math::DivCeil(lightingHeight, 16), 1);
        }
        Graphics::EndComputePass(computePass);
    }
    
    if (spatialFilterSteps % 2 == 0)
    {
        Graphics::CopyTexture(tmpIlluminationTexture, dispatchInfo.unresolvedIllumination);
    }

    Graphics::DestroyBindGroup(temporalAccumBindGroup);
    for (u32 i = 0; i < 2; i++)
    {
        Graphics::DestroyBindGroup(aTrousBindGroup[i]);
    }

    frameIdx++;
}

void ReblurPass::ClearPipelineCache()
{
    ATrousPipeline::Clear();
}