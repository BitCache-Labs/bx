#include "bx/framework/systems/renderer/nert_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/systems/renderer/write_indirect_args_pass.hpp"
#include "bx/framework/systems/renderer/blas_data_pool.hpp"
#include "bx/framework/systems/renderer/material_pool.hpp"
#include "bx/framework/systems/renderer/sky.hpp"
#include "bx/framework/systems/renderer/gbuffer_pass.hpp"
#include "bx/framework/systems/renderer/restir_di_pass.hpp"

#include "bx/framework/components/transform.hpp"
#include "bx/framework/components/mesh_filter.hpp"
#include "bx/framework/components/mesh_renderer.hpp"

#include "bx/engine/core/file.hpp"

struct PackedRay
{
    Vec3 origin;
    u32 direction;
};

struct Intersection
{
    Vec2 uv;
    u32 primitiveIdx;
    u32 blasInstanceIdx;
    f32 t;
    b32 frontFace;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct IntersectConstants
{
    u32 width;
    u32 height;
    u32 maxBounces;
    u32 _PADDING1;
};

struct RaygenConstants
{
    Mat4 invView;
    Mat4 invProj;
    u32 width;
    u32 height;
};

struct SamplegenConstants
{
    u32 width;
    u32 height;
    u32 seed;
    u32 _PADDING0;
};

struct ShadeConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct IntersectPipeline : public LazyInit<IntersectPipeline, ComputePipelineHandle>
{
    IntersectPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Nert Intersect Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/nert/intersect.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                         // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),                                                     // rays
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),                                                    // sampleCount
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),                                                    // intersections
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),                                                    // samplePixelMapping
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),                                                    // inverseSamplePixelMapping
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),                                                 // scene
                BindGroupLayoutEntry(7, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)), // gbuffer
                BindGroupLayoutEntry(8, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)), // neGbuffer
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Nert Intersect Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<IntersectPipeline> LazyInit<IntersectPipeline, ComputePipelineHandle>::cache = nullptr;

struct RaygenPipeline : public LazyInit<RaygenPipeline, ComputePipelineHandle>
{
    RaygenPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Nert Raygen Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/nert/raygen.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true))
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Nert Raygen Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<RaygenPipeline> LazyInit<RaygenPipeline, ComputePipelineHandle>::cache = nullptr;

struct SamplegenPipeline : public LazyInit<SamplegenPipeline, ComputePipelineHandle>
{
    SamplegenPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Nert Samplegen Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/nert/samplegen.comp.shader")));;
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),         // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // rays
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // intersections
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // sampleRayCount
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // samplePixelMapping
            }),
            BlasDataPool::GetBindGroupLayout(),
            MaterialPool::GetBindGroupLayout(),
            Sky::GetBindGroupLayout(),
            Restir::GetBindGroupLayout(),
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Nert Samplegen Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<SamplegenPipeline> LazyInit<SamplegenPipeline, ComputePipelineHandle>::cache = nullptr;

struct ShadePipeline : public LazyInit<ShadePipeline, ComputePipelineHandle>
{
    ShadePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Nert Shade Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/nert/shade.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                             // constants
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),                                                         // inverseSamplePixelMapping
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),                                                         // intersections
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),                                                     // scene
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // neGbuffer
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outImage
            }),
            MaterialPool::GetBindGroupLayout(),
            BlasDataPool::GetBindGroupLayout(),
            Sky::GetBindGroupLayout(),
            Restir::GetBindGroupLayout(),
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Nert Shade Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<ShadePipeline> LazyInit<ShadePipeline, ComputePipelineHandle>::cache = nullptr;

NertPass::NertPass(const NertCreateInfo& createInfo)
    : createInfo(createInfo)
{
    colorTargetView = Graphics::CreateTextureView(createInfo.colorTarget);

    const TextureCreateInfo& colorTargetCreateInfo = Graphics::GetTextureCreateInfo(createInfo.colorTarget);
    BX_ASSERT(colorTargetCreateInfo.format == TextureFormat::RGBA32_FLOAT, "Nert color target must be in TextureFormat::RGBA32_FLOAT.");
    width = colorTargetCreateInfo.size.width;
    height = colorTargetCreateInfo.size.height;

    TextureCreateInfo neGbufferCreateInfo{};
    neGbufferCreateInfo.name = "Nert Non Euclidian GBuffer Texture";
    neGbufferCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    neGbufferCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING;
    neGbufferCreateInfo.size = Extend3D(width, height, 1);
    neGbuffer = Graphics::CreateTexture(neGbufferCreateInfo);
    neGbufferView = Graphics::CreateTextureView(neGbuffer);

    List<u32> pixelMappingData(width * height);
    for (u32 i = 0; i < width * height; i++)
    {
        pixelMappingData[i] = i;
    }
    BufferCreateInfo identityPixelMappingCreateInfo{};
    identityPixelMappingCreateInfo.name = "Nert Identity Pixel Mapping Buffer";
    identityPixelMappingCreateInfo.size = width * height * sizeof(u32);
    identityPixelMappingCreateInfo.data = pixelMappingData.data();
    identityPixelMappingCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    identityPixelMappingBuffer = Graphics::CreateBuffer(identityPixelMappingCreateInfo);

    BufferCreateInfo raysCreateInfo{};
    raysCreateInfo.name = "Nert Rays Buffer";
    raysCreateInfo.size = width * height * sizeof(PackedRay);
    raysCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    raysBuffer = Graphics::CreateBuffer(raysCreateInfo);

    BufferCreateInfo shadowRayCountCreateInfo{};
    shadowRayCountCreateInfo.name = "Nert Sample Count Buffer";
    shadowRayCountCreateInfo.size = sizeof(u32);
    shadowRayCountCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    sampleCountBuffer = Graphics::CreateBuffer(shadowRayCountCreateInfo);

    BufferCreateInfo shadowRayPixelMappingCreateInfo{};
    shadowRayPixelMappingCreateInfo.name = "Nert Sample Pixel Mapping Buffer";
    shadowRayPixelMappingCreateInfo.size = width * height * sizeof(u32);
    shadowRayPixelMappingCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    samplePixelMappingBuffer = Graphics::CreateBuffer(shadowRayPixelMappingCreateInfo);
    shadowRayPixelMappingCreateInfo.name = "Nert Inverse Sample Pixel Mapping Buffer";
    inverseSamplePixelMappingBuffer = Graphics::CreateBuffer(shadowRayPixelMappingCreateInfo);

    BufferCreateInfo intersectionsCreateInfo{};
    intersectionsCreateInfo.name = "Nert Intersections Buffer";
    intersectionsCreateInfo.size = width * height * sizeof(Intersection);
    intersectionsCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    intersectionsBuffer = Graphics::CreateBuffer(intersectionsCreateInfo);

    BufferCreateInfo indirectArgsCreateInfo{};
    indirectArgsCreateInfo.name = "Indirect Args Buffer";
    indirectArgsCreateInfo.size = 3 * sizeof(u32);
    indirectArgsCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::INDIRECT;
    indirectArgsBuffer = Graphics::CreateBuffer(indirectArgsCreateInfo);

    BufferCreateInfo intersectConstantsCreateInfo{};
    intersectConstantsCreateInfo.name = "Nert Intersect Constants Buffer";
    intersectConstantsCreateInfo.size = sizeof(IntersectConstants);
    intersectConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    intersectConstantsBuffer = Graphics::CreateBuffer(intersectConstantsCreateInfo);

    BufferCreateInfo raygenConstantsCreateInfo{};
    raygenConstantsCreateInfo.name = "Nert Raygen Constants Buffer";
    raygenConstantsCreateInfo.size = sizeof(RaygenConstants);
    raygenConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    raygenConstantsBuffer = Graphics::CreateBuffer(raygenConstantsCreateInfo);

    BufferCreateInfo samplegenConstantsCreateInfo{};
    samplegenConstantsCreateInfo.name = "Nert Samplegen Constants Buffer";
    samplegenConstantsCreateInfo.size = sizeof(SamplegenConstants);
    samplegenConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    samplegenConstantsBuffer = Graphics::CreateBuffer(samplegenConstantsCreateInfo);

    BufferCreateInfo shadeConstantsCreateInfo{};
    shadeConstantsCreateInfo.name = "Nert Raygen Constants Buffer";
    shadeConstantsCreateInfo.size = sizeof(ShadeConstants);
    shadeConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    shadeConstantsBuffer = Graphics::CreateBuffer(shadeConstantsCreateInfo);

    gbufferPass = std::unique_ptr<GBufferPass>(new GBufferPass(createInfo.depthTarget));
    restirDiPass = std::unique_ptr<RestirDiPass>(new RestirDiPass(width, height));
}

NertPass::~NertPass()
{
    gbufferPass.reset();
    restirDiPass.reset();

    Graphics::DestroyTextureView(colorTargetView);
    Graphics::DestroyTextureView(neGbufferView);
    Graphics::DestroyTexture(neGbuffer);

    Graphics::DestroyBuffer(raysBuffer);
    Graphics::DestroyBuffer(identityPixelMappingBuffer);
    Graphics::DestroyBuffer(sampleCountBuffer);
    Graphics::DestroyBuffer(samplePixelMappingBuffer);
    Graphics::DestroyBuffer(inverseSamplePixelMappingBuffer);
    Graphics::DestroyBuffer(intersectionsBuffer);
    Graphics::DestroyBuffer(indirectArgsBuffer);

    Graphics::DestroyBuffer(intersectConstantsBuffer);
    Graphics::DestroyBuffer(raygenConstantsBuffer);
    Graphics::DestroyBuffer(samplegenConstantsBuffer);
    Graphics::DestroyBuffer(shadeConstantsBuffer);
}

void NertPass::SetTlas(TlasHandle tlas)
{
    createInfo.tlas = tlas;
}

void NertPass::UpdateConstantBuffers(const NertDispatchInfo& dispatchInfo)
{
    IntersectConstants intersectConstants{};
    intersectConstants.width = width;
    intersectConstants.height = height;
    intersectConstants.maxBounces = maxBounces;
    Graphics::WriteBuffer(intersectConstantsBuffer, 0, &intersectConstants);

    RaygenConstants raygenConstants{};
    raygenConstants.invView = dispatchInfo.camera.GetInvView();
    raygenConstants.invProj = dispatchInfo.camera.GetInvProjection();
    raygenConstants.width = width;
    raygenConstants.height = height;
    Graphics::WriteBuffer(raygenConstantsBuffer, 0, &raygenConstants);

    SamplegenConstants samplegenConstants{};
    samplegenConstants.width = width;
    samplegenConstants.height = height;
    samplegenConstants.seed = seed;
    Graphics::WriteBuffer(samplegenConstantsBuffer, 0, &samplegenConstants);

    ShadeConstants shadeConstants{};
    shadeConstants.width = width;
    shadeConstants.height = height;
    Graphics::WriteBuffer(shadeConstantsBuffer, 0, &shadeConstants);
}

BindGroupHandle NertPass::CreateIntersectBindGroup(const NertDispatchInfo& dispatchInfo)
{
    BindGroupCreateInfo bindGroupCreateInfo{};
    bindGroupCreateInfo.name = "Nert Intersect Bind Group";
    bindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(IntersectPipeline::Get(), 0);
    bindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(intersectConstantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(raysBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(sampleCountBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(intersectionsBuffer)),
        BindGroupEntry(4, BindingResource::Buffer(samplePixelMappingBuffer)),
        BindGroupEntry(5, BindingResource::Buffer(inverseSamplePixelMappingBuffer)),
        BindGroupEntry(6, BindingResource::AccelerationStructure(createInfo.tlas)),
        BindGroupEntry(7, BindingResource::TextureView(dispatchInfo.gbuffer)),
        BindGroupEntry(8, BindingResource::TextureView(neGbufferView)),
    };
    return Graphics::CreateBindGroup(bindGroupCreateInfo);
}

BindGroupHandle NertPass::CreateRaygenBindGroup(const NertDispatchInfo& dispatchInfo)
{
    BindGroupCreateInfo bindGroupCreateInfo{};
    bindGroupCreateInfo.name = "Nert Raygen Bind Group";
    bindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(RaygenPipeline::Get(), 0);
    bindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(raygenConstantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(raysBuffer)),
    };
    return Graphics::CreateBindGroup(bindGroupCreateInfo);
}

BindGroupHandle NertPass::CreateSamplegenBindGroup(const NertDispatchInfo& dispatchInfo)
{
    BindGroupCreateInfo bindGroupCreateInfo{};
    bindGroupCreateInfo.name = "Nert Samplegen Bind Group";
    bindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(SamplegenPipeline::Get(), 0);
    bindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(samplegenConstantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(raysBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(intersectionsBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(sampleCountBuffer)),
        BindGroupEntry(4, BindingResource::Buffer(samplePixelMappingBuffer)),
    };
    return Graphics::CreateBindGroup(bindGroupCreateInfo);
}

BindGroupHandle NertPass::CreateShadeBindGroup(const NertDispatchInfo& dispatchInfo)
{
    BindGroupCreateInfo bindGroupCreateInfo{};
    bindGroupCreateInfo.name = "Nert Shade Bind Group";
    bindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ShadePipeline::Get(), 0);
    bindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(shadeConstantsBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(inverseSamplePixelMappingBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(intersectionsBuffer)),
        BindGroupEntry(4, BindingResource::AccelerationStructure(createInfo.tlas)),
        BindGroupEntry(5, BindingResource::TextureView(neGbufferView)),
        BindGroupEntry(6, BindingResource::TextureView(colorTargetView)),
    };
    return Graphics::CreateBindGroup(bindGroupCreateInfo);
}

void NertPass::Dispatch(const NertDispatchInfo& dispatchInfo)
{
    UpdateConstantBuffers(dispatchInfo);

    BindGroupHandle intersectBindGroup = CreateIntersectBindGroup(dispatchInfo);
    BindGroupHandle raygenBindGroup = CreateRaygenBindGroup(dispatchInfo);
    BindGroupHandle samplegenBindGroup = CreateSamplegenBindGroup(dispatchInfo);
    BindGroupHandle shadeBindGroup = CreateShadeBindGroup(dispatchInfo);

    Graphics::ClearBuffer(sampleCountBuffer);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Nert Raygen";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(RaygenPipeline::Get());
        Graphics::SetBindGroup(0, raygenBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    computePassDescriptor.name = "Nert Intersect";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(IntersectPipeline::Get());
        Graphics::SetBindGroup(0, intersectBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);
    }
    Graphics::EndComputePass(computePass);

    WriteIndirectArgsPass writeIndirectArgs(128);
    writeIndirectArgs.Dispatch(indirectArgsBuffer, sampleCountBuffer);
    
    computePassDescriptor.name = "Nert Samplegen";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        BindGroupHandle blasDataPoolGroup = dispatchInfo.blasDataPool.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle materialPoolGroup = dispatchInfo.materialPool.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle skyGroup = dispatchInfo.sky.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle restirGroup = restirDiPass->CreateBindGroup(ShadePipeline::Get(), true);
    
        Graphics::SetComputePipeline(SamplegenPipeline::Get());
        Graphics::SetBindGroup(0, samplegenBindGroup);
        Graphics::SetBindGroup(BlasDataPool::BIND_GROUP_SET, blasDataPoolGroup);
        Graphics::SetBindGroup(MaterialPool::BIND_GROUP_SET, materialPoolGroup);
        Graphics::SetBindGroup(Sky::BIND_GROUP_SET, skyGroup);
        Graphics::SetBindGroup(Restir::BIND_GROUP_SET, restirGroup);
        Graphics::DispatchWorkgroupsIndirect(indirectArgsBuffer);
    
        Graphics::DestroyBindGroup(blasDataPoolGroup);
        Graphics::DestroyBindGroup(materialPoolGroup);
        Graphics::DestroyBindGroup(skyGroup);
        Graphics::DestroyBindGroup(restirGroup);
    }
    Graphics::EndComputePass(computePass);

    // restirDiPass->seed = seed;
    // restirDiPass->unbiased = unbiased;
    // restirDiPass->jacobian = jacobian;
    // restirDiPass->Dispatch(camera, createInfo.tlas, gbufferPass->GetColorTargetView(), gbufferPass->GetColorTargetHistoryView(), blasDataPool, sky);

    computePassDescriptor.name = "Nert Shade";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        BindGroupHandle blasDataPoolGroup = dispatchInfo.blasDataPool.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle materialPoolGroup = dispatchInfo.materialPool.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle skyGroup = dispatchInfo.sky.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle restirGroup = restirDiPass->CreateBindGroup(ShadePipeline::Get(), true);

        Graphics::SetComputePipeline(ShadePipeline::Get());
        Graphics::SetBindGroup(0, shadeBindGroup);
        Graphics::SetBindGroup(BlasDataPool::BIND_GROUP_SET, blasDataPoolGroup);
        Graphics::SetBindGroup(MaterialPool::BIND_GROUP_SET, materialPoolGroup);
        Graphics::SetBindGroup(Sky::BIND_GROUP_SET, skyGroup);
        Graphics::SetBindGroup(Restir::BIND_GROUP_SET, restirGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);

        Graphics::DestroyBindGroup(blasDataPoolGroup);
        Graphics::DestroyBindGroup(materialPoolGroup);
        Graphics::DestroyBindGroup(skyGroup);
        Graphics::DestroyBindGroup(restirGroup);
    }
    Graphics::EndComputePass(computePass);

    Graphics::DestroyBindGroup(intersectBindGroup);
    Graphics::DestroyBindGroup(raygenBindGroup);
    Graphics::DestroyBindGroup(samplegenBindGroup);
    Graphics::DestroyBindGroup(shadeBindGroup);

    gbufferPass->NextFrame();
}

void NertPass::ClearPipelineCache()
{
    IntersectPipeline::Clear();
    RaygenPipeline::Clear();
    SamplegenPipeline::Clear();
    ShadePipeline::Clear();
}