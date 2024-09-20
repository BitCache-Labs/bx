#include "bx/framework/systems/renderer/restir_di_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

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
    u32 width;
    u32 height;
    u32 seed;
    u32 spatialIndex;
    b32 unbiased;
    u32 _PADDING0;
    u32 _PADDING1;
    u32 _PADDING2;
};

struct TemporalReuseConstants
{
    Mat4 invView;
    Mat4 invProj;
    Mat4 prevInvView;
    Mat4 prevInvProj;
    u32 width;
    u32 height;
    b32 unbiased;
    u32 seed;
};

struct SpatialReusePipeline : public LazyInit<SpatialReusePipeline, ComputePipelineHandle>
{
    SpatialReusePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Restir Spatial Reuse Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/restir/spatial_reuse.shader")));
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
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/restir/temporal_reuse.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
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

RestirDiPass::RestirDiPass(u32 width, u32 height)
    : width(width), height(height)
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

void RestirDiPass::Dispatch(const Camera& camera, TlasHandle tlas, TextureViewHandle gbufferView, TextureViewHandle gbufferHistoryView)
{
    SpatialReuseConstants spatialReuseConstants{};
    spatialReuseConstants.invView = camera.GetInvView();
    spatialReuseConstants.invProj = camera.GetInvProjection();
    spatialReuseConstants.width = width;
    spatialReuseConstants.height = height;
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
    temporalReuseConstants.width = width;
    temporalReuseConstants.height = height;
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
    };
    BindGroupHandle temporalReuseBindGroup = Graphics::CreateBindGroup(temporalReuseBindGroupCreateInfo);

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

    Graphics::DestroyBindGroup(temporalReuseBindGroup);
    return;
    for (u32 i = 0; i < SPATIAL_REUSE_PASSES; i++)
    {
        BindGroupCreateInfo spatialReuseBindGroupCreateInfo{};
        spatialReuseBindGroupCreateInfo.name = "Restir Spatial Reuse Bind Group";
        spatialReuseBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(SpatialReusePipeline::Get(), 0);
        spatialReuseBindGroupCreateInfo.entries = {
            BindGroupEntry(0, BindingResource::Buffer(spatialReuseConstantsBuffers[i])),
            BindGroupEntry(1, BindingResource::TextureView(gbufferView)),
        };
        BindGroupHandle spatialReuseBindGroup = Graphics::CreateBindGroup(spatialReuseBindGroupCreateInfo);

        computePassDescriptor.name = "Restir Spatial Reuse";
        computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(SpatialReusePipeline::Get());
            Graphics::SetBindGroup(0, spatialReuseBindGroup);
            Graphics::SetBindGroup(Restir::BIND_GROUP_SET, restirSpatialBindGroups[i]);
            Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);
        }
        Graphics::EndComputePass(computePass);

        Graphics::DestroyBindGroup(spatialReuseBindGroup);
    }
}

void RestirDiPass::ClearPipelineCache()
{
    SpatialReusePipeline::Clear();
}