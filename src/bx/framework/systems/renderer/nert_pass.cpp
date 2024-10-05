#include "bx/framework/systems/renderer/nert_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/systems/renderer/write_indirect_args_pass.hpp"
#include "bx/framework/systems/renderer/blas_data_pool.hpp"
#include "bx/framework/systems/renderer/material_pool.hpp"
#include "bx/framework/systems/renderer/sky.hpp"
#include "bx/framework/systems/renderer/gbuffer_pass.hpp"
#include "bx/framework/systems/renderer/restir_di_pass.hpp"
#include "bx/framework/systems/renderer/reblur_pass.hpp"

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

struct ResolveConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct SamplegenConstants
{
    u32 width;
    u32 height;
    u32 seed;
    u32 _PADDING0;
};

struct NertShadeConstants
{
    u32 width;
    u32 height;
    u32 sampleNumber;
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

struct ResolvePipeline : public LazyInit<ResolvePipeline, ComputePipelineHandle>
{
    ResolvePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Nert Resolve Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/nert/resolve.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                                 // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ_WRITE, TextureFormat::RGBA32_FLOAT)),   // ambientEmissiveBaseColor
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),         // denoisedIllumination
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),        // outImage
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Nert Resolve Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<ResolvePipeline> LazyInit<ResolvePipeline, ComputePipelineHandle>::cache = nullptr;

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
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()), // scene
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
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                                 // constants
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),                                                             // intersections
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),                                                         // scene
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),         // neGbuffer
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::R32_FLOAT)),           // outIllumination
                BindGroupLayoutEntry(7, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),         // outAmbientEmissiveBaseColor
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
    : createInfo(createInfo), frameIdx(0)
{
    colorTargetView = Graphics::CreateTextureView(createInfo.colorTarget);

    const TextureCreateInfo& colorTargetCreateInfo = Graphics::GetTextureCreateInfo(createInfo.colorTarget);
    BX_ASSERT(colorTargetCreateInfo.format == TextureFormat::RGBA32_FLOAT, "Nert color target must be in TextureFormat::RGBA32_FLOAT.");
    width = colorTargetCreateInfo.size.width;
    height = colorTargetCreateInfo.size.height;

    TextureCreateInfo neGbufferCreateInfo{};
    neGbufferCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    neGbufferCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING;
    neGbufferCreateInfo.size = Extend3D(width, height, 1);
    for (u32 i = 0; i < 2; i++)
    {
        neGbufferCreateInfo.name = Log::Format("Nert Non Euclidian GBuffer {} Texture", i);
        neGbuffer[i] = Graphics::CreateTexture(neGbufferCreateInfo);
        neGbufferView[i] = Graphics::CreateTextureView(neGbuffer[i]);
    }

    TextureCreateInfo illuminationCreateInfo{};
    illuminationCreateInfo.name = "Nert Illumination Texture";
    illuminationCreateInfo.format = TextureFormat::R32_FLOAT;
    illuminationCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::COPY_SRC;
    illuminationCreateInfo.size = Extend3D(width, height, 1);
    illuminationTexture = Graphics::CreateTexture(illuminationCreateInfo);
    illuminationTextureView = Graphics::CreateTextureView(illuminationTexture);

    TextureCreateInfo ambientEmissiveBaseColorCreateInfo{};
    ambientEmissiveBaseColorCreateInfo.name = "Nert Ambient Emissive Base Color Texture";
    ambientEmissiveBaseColorCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    ambientEmissiveBaseColorCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING;
    ambientEmissiveBaseColorCreateInfo.size = Extend3D(width, height, 1);
    ambientEmissiveBaseColorTexture = Graphics::CreateTexture(ambientEmissiveBaseColorCreateInfo);
    ambientEmissiveBaseColorTextureView = Graphics::CreateTextureView(ambientEmissiveBaseColorTexture);

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

    BufferCreateInfo sampleCountCreateInfo{};
    sampleCountCreateInfo.name = "Nert Sample Count Buffer";
    sampleCountCreateInfo.size = sizeof(u32);
    sampleCountCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    sampleCountBuffer = Graphics::CreateBuffer(sampleCountCreateInfo);

    BufferCreateInfo samplePixelMappingCreateInfo{};
    samplePixelMappingCreateInfo.name = "Nert Sample Pixel Mapping Buffer";
    samplePixelMappingCreateInfo.size = width * height * sizeof(u32);
    samplePixelMappingCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    samplePixelMappingBuffer = Graphics::CreateBuffer(samplePixelMappingCreateInfo);

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

    BufferCreateInfo resolveConstantsCreateInfo{};
    resolveConstantsCreateInfo.name = "Nert Resolve Constants Buffer";
    resolveConstantsCreateInfo.size = sizeof(ResolveConstants);
    resolveConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    resolveConstantsBuffer = Graphics::CreateBuffer(resolveConstantsCreateInfo);

    BufferCreateInfo samplegenConstantsCreateInfo{};
    samplegenConstantsCreateInfo.name = "Nert Samplegen Constants Buffer";
    samplegenConstantsCreateInfo.size = sizeof(SamplegenConstants);
    samplegenConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    samplegenConstantsBuffer = Graphics::CreateBuffer(samplegenConstantsCreateInfo);

    BufferCreateInfo shadeConstantsCreateInfo{};
    shadeConstantsCreateInfo.name = "Nert Raygen Constants Buffer";
    shadeConstantsCreateInfo.size = sizeof(NertShadeConstants);
    shadeConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    shadeConstantsBuffer = Graphics::CreateBuffer(shadeConstantsCreateInfo);

    restirDiPass = std::unique_ptr<RestirDiPass>(new RestirDiPass(width, height));
    reblurPass = std::unique_ptr<ReblurPass>(new ReblurPass(width, height));
}

NertPass::~NertPass()
{
    Graphics::DestroyTextureView(colorTargetView);
    for (u32 i = 0; i < 2; i++)
    {
        Graphics::DestroyTextureView(neGbufferView[i]);
        Graphics::DestroyTexture(neGbuffer[i]);
    }

    Graphics::DestroyTextureView(illuminationTextureView);
    Graphics::DestroyTexture(illuminationTexture);
    Graphics::DestroyTextureView(ambientEmissiveBaseColorTextureView);
    Graphics::DestroyTexture(ambientEmissiveBaseColorTexture);

    Graphics::DestroyBuffer(raysBuffer);
    Graphics::DestroyBuffer(identityPixelMappingBuffer);
    Graphics::DestroyBuffer(sampleCountBuffer);
    Graphics::DestroyBuffer(samplePixelMappingBuffer);
    Graphics::DestroyBuffer(intersectionsBuffer);
    Graphics::DestroyBuffer(indirectArgsBuffer);

    Graphics::DestroyBuffer(intersectConstantsBuffer);
    Graphics::DestroyBuffer(raygenConstantsBuffer);
    Graphics::DestroyBuffer(resolveConstantsBuffer);
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

    ResolveConstants resolveConstants{};
    resolveConstants.width = width;
    resolveConstants.height = height;
    Graphics::WriteBuffer(resolveConstantsBuffer, 0, &resolveConstants);

    SamplegenConstants samplegenConstants{};
    samplegenConstants.width = width;
    samplegenConstants.height = height;
    samplegenConstants.seed = seed;
    Graphics::WriteBuffer(samplegenConstantsBuffer, 0, &samplegenConstants);

    NertShadeConstants shadeConstants{};
    shadeConstants.width = width;
    shadeConstants.height = height;
    shadeConstants.sampleNumber = accumulationFrameIdx;
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
        BindGroupEntry(6, BindingResource::AccelerationStructure(createInfo.tlas)),
        BindGroupEntry(7, BindingResource::TextureView(dispatchInfo.gbuffer)),
        BindGroupEntry(8, BindingResource::TextureView(neGbufferView[frameIdx % 2 == 0])),
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

BindGroupHandle NertPass::CreateResolveBindGroup(const NertDispatchInfo& dispatchInfo)
{
    BindGroupCreateInfo bindGroupCreateInfo{};
    bindGroupCreateInfo.name = "Nert Resolve Bind Group";
    bindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ResolvePipeline::Get(), 0);
    bindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(resolveConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(ambientEmissiveBaseColorTextureView)),
        BindGroupEntry(2, BindingResource::TextureView(illuminationTextureView)),
        BindGroupEntry(3, BindingResource::TextureView(colorTargetView)),
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
        BindGroupEntry(5, BindingResource::AccelerationStructure(createInfo.tlas)),
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
        BindGroupEntry(3, BindingResource::Buffer(intersectionsBuffer)),
        BindGroupEntry(4, BindingResource::AccelerationStructure(createInfo.tlas)),
        BindGroupEntry(5, BindingResource::TextureView(neGbufferView[frameIdx % 2 == 0])),
        BindGroupEntry(6, BindingResource::TextureView(illuminationTextureView)),
        BindGroupEntry(7, BindingResource::TextureView(ambientEmissiveBaseColorTextureView)),
    };
    return Graphics::CreateBindGroup(bindGroupCreateInfo);
}

void NertPass::Dispatch(const NertDispatchInfo& dispatchInfo)
{
    UpdateConstantBuffers(dispatchInfo);

    BindGroupHandle intersectBindGroup = CreateIntersectBindGroup(dispatchInfo);
    BindGroupHandle raygenBindGroup = CreateRaygenBindGroup(dispatchInfo);
    BindGroupHandle resolveBindGroup = CreateResolveBindGroup(dispatchInfo);
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
        BindGroupHandle restirGroup = restirDiPass->CreateBindGroup(ShadePipeline::Get(), false);
    
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

    if (restir)
    {
        restirDiPass->seed = seed;
        restirDiPass->unbiased = unbiased;
        restirDiPass->Dispatch(dispatchInfo.camera, createInfo.tlas, dispatchInfo.gbuffer, dispatchInfo.gbufferHistory, dispatchInfo.velocity, dispatchInfo.blasDataPool, dispatchInfo.sky, dispatchInfo.materialPool);
    }

    computePassDescriptor.name = "Nert Shade";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        BindGroupHandle blasDataPoolGroup = dispatchInfo.blasDataPool.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle materialPoolGroup = dispatchInfo.materialPool.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle skyGroup = dispatchInfo.sky.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle restirGroup = restirDiPass->CreateBindGroup(ShadePipeline::Get(), restir ? (RestirDiPass::SPATIAL_REUSE_PASSES % 2 == 0) : true);

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

    if (denoise)
    {
        reblurPass->seed = seed;
        reblurPass->antiFirefly = antiFirefly;

        ReblurDispatchInfo reblurDispatchInfo;
        reblurDispatchInfo.unresolvedIllumination = illuminationTexture;
        reblurDispatchInfo.gbufferView = dispatchInfo.gbuffer;
        reblurDispatchInfo.gbufferHistoryView = dispatchInfo.gbufferHistory;
        reblurDispatchInfo.neGbufferHistoryView = neGbufferView[frameIdx % 2 != 0];
        reblurDispatchInfo.velocityView = dispatchInfo.velocity;
        reblurPass->Dispatch(reblurDispatchInfo);
    }

    computePassDescriptor.name = "Nert Resolve";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(ResolvePipeline::Get());
        Graphics::SetBindGroup(0, resolveBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::DestroyBindGroup(intersectBindGroup);
    Graphics::DestroyBindGroup(raygenBindGroup);
    Graphics::DestroyBindGroup(resolveBindGroup);
    Graphics::DestroyBindGroup(samplegenBindGroup);
    Graphics::DestroyBindGroup(shadeBindGroup);

    frameIdx++;
}

void NertPass::ClearPipelineCache()
{
    IntersectPipeline::Clear();
    RaygenPipeline::Clear();
    SamplegenPipeline::Clear();
    ShadePipeline::Clear();
}