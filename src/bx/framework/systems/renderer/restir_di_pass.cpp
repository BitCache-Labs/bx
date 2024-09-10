#include "bx/framework/systems/renderer/restir_di_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/engine/core/file.hpp"

BindGroupLayoutDescriptor Restir::GetBindGroupLayout()
{
    return BindGroupLayoutDescriptor(BIND_GROUP_SET, {
            BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),
            BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
            BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
        });
}

struct SpatialReuseConstants
{
    u32 dispatchSize;
    u32 seed;
    u32 width;
    u32 pixelRadius;
};

struct TemporalReuseConstants
{
    u32 dispatchSize;
    u32 seed;
    u32 _PADDING1;
    u32 _PADDING2;
};

struct SpatialReusePipeline : public LazyInit<SpatialReusePipeline, ComputePipelineHandle>
{
    SpatialReusePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Restir Spatial Reuse Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/restir/spatial_reuse.shader")));;
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
            }),
            Restir::GetBindGroupLayout()
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Restir Spatial Reuse Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<SpatialReusePipeline> LazyInit<SpatialReusePipeline, ComputePipelineHandle>::cache = nullptr;

struct TemporalReusePipeline : public LazyInit<TemporalReusePipeline, ComputePipelineHandle>
{
    TemporalReusePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Restir Temporal Reuse Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/restir/temporal_reuse.shader")));;
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),
            }),
            Restir::GetBindGroupLayout()
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Restir Temporal Reuse Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<TemporalReusePipeline> LazyInit<TemporalReusePipeline, ComputePipelineHandle>::cache = nullptr;

RestirDiPass::RestirDiPass(u32 width, u32 height, TlasHandle tlas, TextureViewHandle gbufferView)
    : width(width), height(height)
{
    BufferCreateInfo restirSamplesCreateInfo{};
    restirSamplesCreateInfo.name = "Restir Samples Buffer";
    restirSamplesCreateInfo.size = width * height * sizeof(Restir::RestirSample);
    restirSamplesCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    samplesBuffer = Graphics::CreateBuffer(restirSamplesCreateInfo);

    BufferCreateInfo restirOutSamplesCreateInfo{};
    restirOutSamplesCreateInfo.name = "Restir Out Samples Buffer";
    restirOutSamplesCreateInfo.size = width * height * sizeof(Restir::RestirSample);
    restirOutSamplesCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    outSamplesBuffer = Graphics::CreateBuffer(restirOutSamplesCreateInfo);

    BufferCreateInfo restirSamplesHistoryCreateInfo{};
    restirSamplesHistoryCreateInfo.name = "Restir Samples History Buffer";
    restirSamplesHistoryCreateInfo.size = width * height * sizeof(Restir::RestirSample);
    restirSamplesHistoryCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    samplesHistoryBuffer = Graphics::CreateBuffer(restirSamplesHistoryCreateInfo);

    BufferCreateInfo spatialReuseConstantsCreateInfo{};
    spatialReuseConstantsCreateInfo.name = "Restir Spatial Reuse Constants Buffer";
    spatialReuseConstantsCreateInfo.size = sizeof(SpatialReuseConstants);
    spatialReuseConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        spatialReuseConstantsBuffers[i] = Graphics::CreateBuffer(spatialReuseConstantsCreateInfo);
    }

    BufferCreateInfo temporalReuseConstantsCreateInfo{};
    temporalReuseConstantsCreateInfo.name = "Restir Temporal Reuse Constants Buffer";
    temporalReuseConstantsCreateInfo.size = sizeof(TemporalReuseConstants);
    temporalReuseConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    temporalReuseConstantsBuffer = Graphics::CreateBuffer(temporalReuseConstantsCreateInfo);

    BindGroupCreateInfo spatialReuseBindGroupCreateInfo{};
    spatialReuseBindGroupCreateInfo.name = "Restir Spatial Reuse Bind Group";
    spatialReuseBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(SpatialReusePipeline::Get(), 0);
    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        spatialReuseBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(spatialReuseConstantsBuffers[i])),
        BindGroupEntry(1, BindingResource::TextureView(gbufferView)),
        };
        spatialReuseBindGroups[i] = Graphics::CreateBindGroup(spatialReuseBindGroupCreateInfo);
    }

    BindGroupCreateInfo temporalReuseBindGroupCreateInfo{};
    temporalReuseBindGroupCreateInfo.name = "Restir Temporal Reuse Bind Group";
    temporalReuseBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(TemporalReusePipeline::Get(), 0);
    temporalReuseBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(temporalReuseConstantsBuffer)),
        BindGroupEntry(1, BindingResource::AccelerationStructure(tlas)),
    };
    temporalReuseBindGroup = Graphics::CreateBindGroup(temporalReuseBindGroupCreateInfo);

    restirTemporalBindGroup = CreateBindGroup(TemporalReusePipeline::Get(), true);
    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        restirSpatialBindGroups[i] = CreateBindGroup(SpatialReusePipeline::Get(), i % 2 == 0);
    }
}

RestirDiPass::~RestirDiPass()
{
    Graphics::DestroyBuffer(samplesBuffer);
    Graphics::DestroyBuffer(outSamplesBuffer);
    Graphics::DestroyBuffer(samplesHistoryBuffer);

    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        Graphics::DestroyBuffer(spatialReuseConstantsBuffers[i]);
    }
    Graphics::DestroyBuffer(temporalReuseConstantsBuffer);

    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        Graphics::DestroyBindGroup(spatialReuseBindGroups[i]);
    }
    Graphics::DestroyBindGroup(temporalReuseBindGroup);
}

BindGroupHandle RestirDiPass::CreateBindGroup(ComputePipelineHandle pipeline, b8 flipRestirSamples) const
{
    BindGroupCreateInfo createInfo{};
    createInfo.name = "Restir Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(pipeline, Restir::BIND_GROUP_SET);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(!flipRestirSamples ? samplesBuffer : outSamplesBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(!flipRestirSamples ? outSamplesBuffer : samplesBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(samplesHistoryBuffer))
    };
    return Graphics::CreateBindGroup(createInfo);
}

void RestirDiPass::Dispatch()
{
    SpatialReuseConstants spatialReuseConstants{};
    spatialReuseConstants.dispatchSize = width * height;
    spatialReuseConstants.width = width;
    spatialReuseConstants.pixelRadius = pixelRadius;
    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        spatialReuseConstants.seed = seed ^ (i * 271983);
        Graphics::WriteBuffer(spatialReuseConstantsBuffers[i], 0, &spatialReuseConstants);
    }

    TemporalReuseConstants temporalReuseConstants{};
    temporalReuseConstants.dispatchSize = width * height;
    temporalReuseConstants.seed = seed;
    Graphics::WriteBuffer(temporalReuseConstantsBuffer, 0, &temporalReuseConstants);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Restir Temporal Reuse";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(TemporalReusePipeline::Get());
        Graphics::SetBindGroup(0, temporalReuseBindGroup);
        Graphics::SetBindGroup(Restir::BIND_GROUP_SET, restirTemporalBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);
    }
    Graphics::EndComputePass(computePass);

    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        computePassDescriptor.name = "Restir Spatial Reuse";
        computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(SpatialReusePipeline::Get());
            Graphics::SetBindGroup(0, spatialReuseBindGroups[i]);
            Graphics::SetBindGroup(Restir::BIND_GROUP_SET, restirSpatialBindGroups[i]);
            Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);
        }
        Graphics::EndComputePass(computePass);
    }
}

void RestirDiPass::ClearPipelineCache()
{
    SpatialReusePipeline::Clear();
}