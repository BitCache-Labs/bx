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

RestirDiPass::RestirDiPass(u32 width, u32 height)
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
    spatialReuseConstantsBuffer = Graphics::CreateBuffer(spatialReuseConstantsCreateInfo);

    BindGroupCreateInfo spatialReuseBindGroupCreateInfo{};
    spatialReuseBindGroupCreateInfo.name = "Restir Spatial Reuse Bind Group";
    spatialReuseBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(SpatialReusePipeline::Get(), 0);
    spatialReuseBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(spatialReuseConstantsBuffer)),
    };
    spatialReuseBindGroup = Graphics::CreateBindGroup(spatialReuseBindGroupCreateInfo);

    restirBindGroup = CreateBindGroup(SpatialReusePipeline::Get(), true);
}

RestirDiPass::~RestirDiPass()
{
    Graphics::DestroyBuffer(samplesBuffer);
    Graphics::DestroyBuffer(outSamplesBuffer);
    Graphics::DestroyBuffer(samplesHistoryBuffer);

    Graphics::DestroyBuffer(spatialReuseConstantsBuffer);

	Graphics::DestroyBindGroup(spatialReuseBindGroup);
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
    spatialReuseConstants.seed = seed;
    spatialReuseConstants.width = width;
    spatialReuseConstants.pixelRadius = pixelRadius;
    Graphics::WriteBuffer(spatialReuseConstantsBuffer, 0, &spatialReuseConstants);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Restir Spatial Reuse";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(SpatialReusePipeline::Get());
        Graphics::SetBindGroup(0, spatialReuseBindGroup);
        Graphics::SetBindGroup(Restir::BIND_GROUP_SET, restirBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);
    }
    Graphics::EndComputePass(computePass);
}

void RestirDiPass::ClearPipelineCache()
{
    SpatialReusePipeline::Clear();
}