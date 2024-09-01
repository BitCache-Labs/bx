#include "bx/framework/systems/renderer/restir_di_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/engine/core/file.hpp"

struct SpatialReuseConstants
{
    u32 dispatchSize;
    u32 _PADDING0;
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
            }),
            BindGroupLayoutDescriptor(3, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
            })
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

RestirDiPass::RestirDiPass(BufferHandle samplesBuffer, BufferHandle samplesHistoryBuffer)
{
    dispatchSize = Graphics::GetBufferCreateInfo(samplesBuffer).size / sizeof(RestirSample);

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

    BindGroupCreateInfo restirBindGroupCreateInfo{};
    restirBindGroupCreateInfo.name = "Restir Bind Group";
    restirBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(SpatialReusePipeline::Get(), 3);
    restirBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(samplesBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(samplesHistoryBuffer))
    };
    restirBindGroup = Graphics::CreateBindGroup(restirBindGroupCreateInfo);
}

RestirDiPass::~RestirDiPass()
{
    Graphics::DestroyBuffer(spatialReuseConstantsBuffer);

	Graphics::DestroyBindGroup(spatialReuseBindGroup);
}

void RestirDiPass::Dispatch()
{
    return;
    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Restir Spatial Reuse";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(SpatialReusePipeline::Get());
        Graphics::SetBindGroup(0, spatialReuseBindGroup);
        Graphics::SetBindGroup(3, restirBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(dispatchSize, 128), 1, 1);
    }
    Graphics::EndComputePass(computePass);
}

void RestirDiPass::ClearPipelineCache()
{
    SpatialReusePipeline::Clear();
}