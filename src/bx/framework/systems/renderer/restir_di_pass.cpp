#include "bx/framework/systems/renderer/restir_di_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"
#include "bx/framework/systems/renderer/sky.hpp"
#include "bx/framework/systems/renderer/blas_data_pool.hpp"
#include "bx/framework/systems/renderer/material_pool.hpp"

#include "bx/engine/core/file.hpp"

BindGroupLayoutDescriptor Restir::GetBindGroupLayout()
{
    return BindGroupLayoutDescriptor(BIND_GROUP_SET, {
            BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
            BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
            BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
            BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
            BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
            BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
        });
}

struct SpatialReuseConstants
{
    Mat4 invView;
    Mat4 invProj;
    u32 globalWidth;
    u32 globalHeight;
    u32 width;
    u32 height;
    u32 seed;
    u32 spatialIndex;
    b32 unbiased;
    u32 _PADDING0;
};

struct TemporalReuseConstants
{
    Mat4 invView;
    Mat4 invProj;
    Mat4 prevInvView;
    Mat4 prevInvProj;
    u32 globalWidth;
    u32 globalHeight;
    u32 width;
    u32 height;
    b32 unbiased;
    u32 seed;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct SpatialReusePipeline : public LazyInit<SpatialReusePipeline, ComputePipelineHandle>
{
    SpatialReusePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Restir Spatial Reuse Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/restir_di/spatial_reuse.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler())
            }),
            BlasDataPool::GetBindGroupLayout(),
            Sky::GetBindGroupLayout(),
            Restir::GetBindGroupLayout(),
            MaterialPool::GetBindGroupLayout()
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
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/restir_di/temporal_reuse.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler())
            }),
            BlasDataPool::GetBindGroupLayout(),
            Sky::GetBindGroupLayout(),
            Restir::GetBindGroupLayout(),
            MaterialPool::GetBindGroupLayout()
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

RestirDiPass::RestirDiPass(u32 width, u32 height, u32 lightingWidth, u32 lightingHeight)
    : width(width), height(height), lightingWidth(lightingWidth), lightingHeight(lightingHeight)
{
    BufferCreateInfo restirSamplesCreateInfo{};
    restirSamplesCreateInfo.name = "Restir Reservoirs Buffer";
    restirSamplesCreateInfo.size = width * height * sizeof(Restir::PackedReservoir);
    restirSamplesCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    reservoirsBuffer = Graphics::CreateBuffer(restirSamplesCreateInfo);

    BufferCreateInfo restirOutSamplesCreateInfo{};
    restirOutSamplesCreateInfo.name = "Restir Out Reservoirs Buffer";
    restirOutSamplesCreateInfo.size = width * height * sizeof(Restir::PackedReservoir);
    restirOutSamplesCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    outReservoirsBuffer = Graphics::CreateBuffer(restirOutSamplesCreateInfo);

    BufferCreateInfo restirSamplesHistoryCreateInfo{};
    restirSamplesHistoryCreateInfo.name = "Restir Reservoirs History Buffer";
    restirSamplesHistoryCreateInfo.size = width * height * sizeof(Restir::PackedReservoir);
    restirSamplesHistoryCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    reservoirsHistoryBuffer = Graphics::CreateBuffer(restirSamplesHistoryCreateInfo);

    BufferCreateInfo restirReservoirDataCreateInfo{};
    restirReservoirDataCreateInfo.name = "Restir Reservoir Data Buffer";
    restirReservoirDataCreateInfo.size = width * height * sizeof(Restir::PackedReservoirData);
    restirReservoirDataCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    reservoirDataBuffer = Graphics::CreateBuffer(restirReservoirDataCreateInfo);

    BufferCreateInfo restirOutReservoirDataCreateInfo{};
    restirOutReservoirDataCreateInfo.name = "Restir Out Reservoir Data Buffer";
    restirOutReservoirDataCreateInfo.size = width * height * sizeof(Restir::PackedReservoirData);
    restirOutReservoirDataCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    outReservoirDataBuffer = Graphics::CreateBuffer(restirOutReservoirDataCreateInfo);

    BufferCreateInfo restirReservoirDataHistoryCreateInfo{};
    restirReservoirDataHistoryCreateInfo.name = "Restir Reservoir Data History Buffer";
    restirReservoirDataHistoryCreateInfo.size = width * height * sizeof(Restir::PackedReservoirData);
    restirReservoirDataHistoryCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    reservoirDataHistoryBuffer = Graphics::CreateBuffer(restirReservoirDataHistoryCreateInfo);

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

    restirTemporalBindGroup = CreateBindGroup(TemporalReusePipeline::Get(), true);
    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        restirSpatialBindGroups[i] = CreateBindGroup(SpatialReusePipeline::Get(), i % 2 == 0);
    }

    SamplerCreateInfo pointClampCreateInfo{};
    pointClampCreateInfo.name = "Restir Point Clamp Sampler";
    pointClampCreateInfo.addressModeU = SamplerAddressMode::CLAMP_TO_EDGE;
    pointClampCreateInfo.addressModeV = SamplerAddressMode::CLAMP_TO_EDGE;
    pointClampCreateInfo.addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
    pointClampCreateInfo.minFilter = FilterMode::NEAREST;
    pointClampCreateInfo.magFilter = FilterMode::NEAREST;
    nearestClampSampler = Graphics::CreateSampler(pointClampCreateInfo);
}

RestirDiPass::~RestirDiPass()
{
    Graphics::DestroyBuffer(reservoirsBuffer);
    Graphics::DestroyBuffer(outReservoirsBuffer);
    Graphics::DestroyBuffer(reservoirsHistoryBuffer);
    Graphics::DestroyBuffer(reservoirDataBuffer);
    Graphics::DestroyBuffer(outReservoirDataBuffer);
    Graphics::DestroyBuffer(reservoirDataHistoryBuffer);

    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        Graphics::DestroyBuffer(spatialReuseConstantsBuffers[i]);
    }
    Graphics::DestroyBuffer(temporalReuseConstantsBuffer);

    Graphics::DestroySampler(nearestClampSampler);
}

BindGroupHandle RestirDiPass::CreateBindGroup(ComputePipelineHandle pipeline, b8 flipRestirSamples) const
{
    BindGroupCreateInfo createInfo{};
    createInfo.name = "Restir Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(pipeline, Restir::BIND_GROUP_SET);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(!flipRestirSamples ? reservoirsBuffer : outReservoirsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(!flipRestirSamples ? outReservoirsBuffer : reservoirsBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(reservoirsHistoryBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(!flipRestirSamples ? reservoirDataBuffer : outReservoirDataBuffer)),
        BindGroupEntry(4, BindingResource::Buffer(!flipRestirSamples ? outReservoirDataBuffer : reservoirDataBuffer)),
        BindGroupEntry(5, BindingResource::Buffer(reservoirDataHistoryBuffer))
    };
    return Graphics::CreateBindGroup(createInfo);
}

void RestirDiPass::Dispatch(const Camera& camera, TlasHandle tlas, TextureViewHandle gbufferView, TextureViewHandle gbufferHistoryView, TextureViewHandle velocityView, TextureViewHandle neGbufferView, const BlasDataPool& blasDataPool, const Sky& sky, const MaterialPool& materialPool)
{
    SpatialReuseConstants spatialReuseConstants{};
    spatialReuseConstants.invView = camera.GetInvView();
    spatialReuseConstants.invProj = camera.GetInvProjection();
    spatialReuseConstants.globalWidth = width;
    spatialReuseConstants.globalHeight = height;
    spatialReuseConstants.width = lightingWidth;
    spatialReuseConstants.height = lightingHeight;
    spatialReuseConstants.seed = seed;
    spatialReuseConstants.unbiased = unbiased;
    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        spatialReuseConstants.spatialIndex = i;
        Graphics::WriteBuffer(spatialReuseConstantsBuffers[i], 0, &spatialReuseConstants);
    }

    TemporalReuseConstants temporalReuseConstants{};
    temporalReuseConstants.invView = camera.GetInvView();
    temporalReuseConstants.invProj = camera.GetInvProjection();
    temporalReuseConstants.prevInvView = camera.GetPrevInvView();
    temporalReuseConstants.prevInvProj = camera.GetPrevInvProjection();
    temporalReuseConstants.globalWidth = width;
    temporalReuseConstants.globalHeight = height;
    temporalReuseConstants.width = lightingWidth;
    temporalReuseConstants.height = lightingHeight;
    temporalReuseConstants.unbiased = unbiased;
    temporalReuseConstants.seed = seed;
    Graphics::WriteBuffer(temporalReuseConstantsBuffer, 0, &temporalReuseConstants);

    BindGroupCreateInfo temporalReuseBindGroupCreateInfo{};
    temporalReuseBindGroupCreateInfo.name = "Restir Temporal Reuse Bind Group";
    temporalReuseBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(TemporalReusePipeline::Get(), 0);
    temporalReuseBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(temporalReuseConstantsBuffer)),
        BindGroupEntry(1, BindingResource::AccelerationStructure(tlas)),
        BindGroupEntry(2, BindingResource::TextureView(gbufferView)),
        BindGroupEntry(3, BindingResource::TextureView(gbufferHistoryView)),
        BindGroupEntry(4, BindingResource::TextureView(velocityView)),
        BindGroupEntry(5, BindingResource::TextureView(neGbufferView)),
        BindGroupEntry(6, BindingResource::Sampler(nearestClampSampler)),
    };
    BindGroupHandle temporalReuseBindGroup = Graphics::CreateBindGroup(temporalReuseBindGroupCreateInfo);
    BindGroupHandle temporalBlasDataPoolGroup = blasDataPool.CreateBindGroup(TemporalReusePipeline::Get());
    BindGroupHandle temporalSkyGroup = sky.CreateBindGroup(TemporalReusePipeline::Get());
    BindGroupHandle temporalMaterialPoolGroup = materialPool.CreateBindGroup(TemporalReusePipeline::Get());

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Restir Temporal Reuse";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(TemporalReusePipeline::Get());
        Graphics::SetBindGroup(0, temporalReuseBindGroup);
        Graphics::SetBindGroup(BlasDataPool::BIND_GROUP_SET, temporalBlasDataPoolGroup);
        Graphics::SetBindGroup(Sky::BIND_GROUP_SET, temporalSkyGroup);
        Graphics::SetBindGroup(Restir::BIND_GROUP_SET, restirTemporalBindGroup);
        Graphics::SetBindGroup(MaterialPool::BIND_GROUP_SET, temporalMaterialPoolGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(lightingWidth * lightingHeight, 128), 1, 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::DestroyBindGroup(temporalReuseBindGroup);
    Graphics::DestroyBindGroup(temporalBlasDataPoolGroup);
    Graphics::DestroyBindGroup(temporalSkyGroup);
    Graphics::DestroyBindGroup(temporalMaterialPoolGroup);
    return;
    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        BindGroupCreateInfo spatialReuseBindGroupCreateInfo{};
        spatialReuseBindGroupCreateInfo.name = "Restir Spatial Reuse Bind Group";
        spatialReuseBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(SpatialReusePipeline::Get(), 0);
        spatialReuseBindGroupCreateInfo.entries = {
            BindGroupEntry(0, BindingResource::Buffer(spatialReuseConstantsBuffers[i])),
            BindGroupEntry(1, BindingResource::TextureView(gbufferView)),
            BindGroupEntry(2, BindingResource::AccelerationStructure(tlas)),
            BindGroupEntry(3, BindingResource::TextureView(neGbufferView)),
            BindGroupEntry(4, BindingResource::Sampler(nearestClampSampler)),
        };
        BindGroupHandle spatialReuseBindGroup = Graphics::CreateBindGroup(spatialReuseBindGroupCreateInfo);
        BindGroupHandle spatialBlasDataPoolGroup = blasDataPool.CreateBindGroup(SpatialReusePipeline::Get());
        BindGroupHandle spatialSkyGroup = sky.CreateBindGroup(SpatialReusePipeline::Get());
        BindGroupHandle spatialMaterialPoolGroup = materialPool.CreateBindGroup(SpatialReusePipeline::Get());
    
        computePassDescriptor.name = "Restir Spatial Reuse";
        computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(SpatialReusePipeline::Get());
            Graphics::SetBindGroup(0, spatialReuseBindGroup);
            Graphics::SetBindGroup(BlasDataPool::BIND_GROUP_SET, spatialBlasDataPoolGroup);
            Graphics::SetBindGroup(Sky::BIND_GROUP_SET, spatialSkyGroup);
            Graphics::SetBindGroup(Restir::BIND_GROUP_SET, restirSpatialBindGroups[i]);
            Graphics::SetBindGroup(MaterialPool::BIND_GROUP_SET, spatialMaterialPoolGroup);
            Graphics::DispatchWorkgroups(Math::DivCeil(lightingWidth * lightingHeight, 128), 1, 1);
        }
        Graphics::EndComputePass(computePass);
    
        Graphics::DestroyBindGroup(spatialReuseBindGroup);
        Graphics::DestroyBindGroup(spatialBlasDataPoolGroup);
        Graphics::DestroyBindGroup(spatialSkyGroup);
        Graphics::DestroyBindGroup(spatialMaterialPoolGroup);
    }
}

void RestirDiPass::ClearPipelineCache()
{
    SpatialReusePipeline::Clear();
}