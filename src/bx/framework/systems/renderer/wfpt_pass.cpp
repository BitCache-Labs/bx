#include "bx/framework/systems/renderer/wfpt_pass.hpp"

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

struct Ray
{
    Vec3 origin;
    u32 _PADDING0;
    Vec3 direction;
    u32 _PADDING1;
};

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

struct Payload
{
    u32 accumulated;
    u32 throughput;
    u32 rngState;
    u32 hitNormal;
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
    u32 sampleNumber;
    u32 _PADDING0;
};

struct ShadeConstants
{
    u32 width;
    u32 height;
    u32 bounce;
    u32 seed;
    b32 russianRoulette;
    b32 hybrid;
    u32 _PADDING1;
    u32 _PADDING2;
};

struct ConnectPipeline : public LazyInit<ConnectPipeline, ComputePipelineHandle>
{
    ConnectPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Wfpt Connect Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/wfpt/connect.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // shadowRayOrigins
                //BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // shadowRayDistances
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // shadowRayCount
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),    // payloads
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // shadowPixelMapping
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()), // scene
            }),
            BlasDataPool::GetBindGroupLayout(),
            Sky::GetBindGroupLayout(),
            Restir::GetBindGroupLayout(),
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Wfpt Connect Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<ConnectPipeline> LazyInit<ConnectPipeline, ComputePipelineHandle>::cache = nullptr;

struct ExtendPipeline : public LazyInit<ExtendPipeline, ComputePipelineHandle>
{
    ExtendPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Wfpt Extend Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/wfpt/extend.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Wfpt Extend Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<ExtendPipeline> LazyInit<ExtendPipeline, ComputePipelineHandle>::cache = nullptr;

struct RaygenPipeline : public LazyInit<RaygenPipeline, ComputePipelineHandle>
{
    RaygenPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Wfpt Raygen Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/wfpt/raygen.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true))
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Wfpt Raygen Pipeline";
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
        shaderCreateInfo.name = "Wfpt Resolve Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/wfpt/resolve.comp.shader")));;
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT))
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Wfpt Resolve Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<ResolvePipeline> LazyInit<ResolvePipeline, ComputePipelineHandle>::cache = nullptr;

struct ShadePipeline : public LazyInit<ShadePipeline, ComputePipelineHandle>
{
    ShadePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Wfpt Shade Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/wfpt/shade.comp.shader")));;
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),         // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // rays
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),    // outRays
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // rayCount
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),    // outRayCount
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // pixelMapping
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),    // outPixelMapping
                BindGroupLayoutEntry(7, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),    // payloads
                BindGroupLayoutEntry(8, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // intersections
                BindGroupLayoutEntry(9, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),    // shadowRayOrigins
                //BindGroupLayoutEntry(10, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),   // shadowRayDistances
                BindGroupLayoutEntry(11, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),   // shadowRayCount
                BindGroupLayoutEntry(12, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),   // shadowPixelMapping
                BindGroupLayoutEntry(13, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),   // gbuffer
            }),
            BlasDataPool::GetBindGroupLayout(),
            MaterialPool::GetBindGroupLayout(),
            Sky::GetBindGroupLayout(),
            Restir::GetBindGroupLayout(),
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Wfpt Shade Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<ShadePipeline> LazyInit<ShadePipeline, ComputePipelineHandle>::cache = nullptr;

WfptPass::WfptPass(const WfptCreateInfo& createInfo)
    : createInfo(createInfo)
{
    colorTargetView = Graphics::CreateTextureView(createInfo.colorTarget);

    const TextureCreateInfo& colorTargetCreateInfo = Graphics::GetTextureCreateInfo(createInfo.colorTarget);
    BX_ASSERT(colorTargetCreateInfo.format == TextureFormat::RGBA32_FLOAT, "Wfpt color target must be in TextureFormat::RGBA32_FLOAT.");
    width = colorTargetCreateInfo.size.width;
    height = colorTargetCreateInfo.size.height;

    List<u32> pixelMappingData(width * height);
    for (u32 i = 0; i < width * height; i++)
    {
        pixelMappingData[i] = i;
    }
    BufferCreateInfo identityPixelMappingCreateInfo{};
    identityPixelMappingCreateInfo.name = "Wfpt Identity Pixel Mapping Buffer";
    identityPixelMappingCreateInfo.size = width * height * sizeof(u32);
    identityPixelMappingCreateInfo.data = pixelMappingData.data();
    identityPixelMappingCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    identityPixelMappingBuffer = Graphics::CreateBuffer(identityPixelMappingCreateInfo);

    for (u32 i = 0; i < 2; i++)
    {
        BufferCreateInfo raysCreateInfo{};
        raysCreateInfo.name = Log::Format("Wfpt Rays {} Buffer", i);
        raysCreateInfo.size = width * height * sizeof(PackedRay);
        raysCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
        raysBuffer[i] = Graphics::CreateBuffer(raysCreateInfo);

        BufferCreateInfo rayCountCreateInfo{};
        rayCountCreateInfo.name = Log::Format("Wfpt Ray Count {} Buffer", i);
        rayCountCreateInfo.size = sizeof(u32);
        rayCountCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
        rayCountBuffer[i] = Graphics::CreateBuffer(rayCountCreateInfo);

        BufferCreateInfo pixelMappingCreateInfo{};
        pixelMappingCreateInfo.name = Log::Format("Wfpt Pixel Mapping {} Buffer", i);
        pixelMappingCreateInfo.size = width * height * sizeof(u32);
        pixelMappingCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
        pixelMappingBuffer[i] = Graphics::CreateBuffer(pixelMappingCreateInfo);
    }

    BufferCreateInfo shadowRayOriginsCreateInfo{};
    shadowRayOriginsCreateInfo.name = "Wfpt Shadow Ray Origins Buffer";
    shadowRayOriginsCreateInfo.size = width * height * sizeof(Vec4);
    shadowRayOriginsCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    shadowRayOriginsBuffer = Graphics::CreateBuffer(shadowRayOriginsCreateInfo);

    BufferCreateInfo shadowRayCountCreateInfo{};
    shadowRayCountCreateInfo.name = "Wfpt Shadow Ray Count Buffer";
    shadowRayCountCreateInfo.size = sizeof(u32);
    shadowRayCountCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    shadowRayCountBuffer = Graphics::CreateBuffer(shadowRayCountCreateInfo);

    BufferCreateInfo shadowRayPixelMappingCreateInfo{};
    shadowRayPixelMappingCreateInfo.name = "Wfpt Shadow Ray Pixel Mapping Buffer";
    shadowRayPixelMappingCreateInfo.size = width * height * sizeof(u32);
    shadowRayPixelMappingCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    shadowRayPixelMappingBuffer = Graphics::CreateBuffer(shadowRayPixelMappingCreateInfo);

    BufferCreateInfo intersectionsCreateInfo{};
    intersectionsCreateInfo.name = "Wfpt Intersections Buffer";
    intersectionsCreateInfo.size = width * height * sizeof(Intersection);
    intersectionsCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    intersectionsBuffer = Graphics::CreateBuffer(intersectionsCreateInfo);

    BufferCreateInfo payloadsCreateInfo{};
    payloadsCreateInfo.name = "Wfpt Payloads Buffer";
    payloadsCreateInfo.size = width * height * sizeof(Payload);
    payloadsCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    payloadsBuffer = Graphics::CreateBuffer(payloadsCreateInfo);

    BufferCreateInfo indirectArgsCreateInfo{};
    indirectArgsCreateInfo.name = "Indirect Args Buffer";
    indirectArgsCreateInfo.size = 3 * sizeof(u32);
    indirectArgsCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::INDIRECT;
    indirectArgsBuffer = Graphics::CreateBuffer(indirectArgsCreateInfo);

    BufferCreateInfo raygenConstantsCreateInfo{};
    raygenConstantsCreateInfo.name = "Wfpt Raygen Constants Buffer";
    raygenConstantsCreateInfo.size = sizeof(RaygenConstants);
    raygenConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    raygenConstantsBuffer = Graphics::CreateBuffer(raygenConstantsCreateInfo);

    BufferCreateInfo resolveConstantsCreateInfo{};
    resolveConstantsCreateInfo.name = "Wfpt Resolve Constants Buffer";
    resolveConstantsCreateInfo.size = sizeof(ResolveConstants);
    resolveConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    resolveConstantsBuffer = Graphics::CreateBuffer(resolveConstantsCreateInfo);

    BindGroupCreateInfo connectBindGroupCreateInfo{};
    connectBindGroupCreateInfo.name = "Wfpt Connect Bind Group";
    connectBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ConnectPipeline::Get(), 0);
    connectBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(shadowRayOriginsBuffer)),
        //BindGroupEntry(1, BindingResource::Buffer(shadowRayDistancesBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(shadowRayCountBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(payloadsBuffer)),
        BindGroupEntry(4, BindingResource::Buffer(shadowRayPixelMappingBuffer)),
        BindGroupEntry(5, BindingResource::AccelerationStructure(createInfo.tlas)),
    };
    connectBindGroup = Graphics::CreateBindGroup(connectBindGroupCreateInfo);

    BindGroupCreateInfo raygenBindGroupCreateInfo{};
    raygenBindGroupCreateInfo.name = "Wfpt Raygen Bind Group";
    raygenBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(RaygenPipeline::Get(), 0);
    raygenBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(raygenConstantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(raysBuffer[0]))
    };
    raygenBindGroup = Graphics::CreateBindGroup(raygenBindGroupCreateInfo);

    BindGroupCreateInfo resolveBindGroupCreateInfo{};
    resolveBindGroupCreateInfo.name = "Wfpt Resolve Bind Group";
    resolveBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ResolvePipeline::Get(), 0);
    resolveBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(resolveConstantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(payloadsBuffer)),
        BindGroupEntry(2, BindingResource::TextureView(colorTargetView))
    };
    resolveBindGroup = Graphics::CreateBindGroup(resolveBindGroupCreateInfo);

    gbufferPass = std::unique_ptr<GBufferPass>(new GBufferPass(createInfo.depthTarget));
    restirDiPass = std::unique_ptr<RestirDiPass>(new RestirDiPass(width, height));
}

WfptPass::~WfptPass()
{
    restirDiPass.reset();

    Graphics::DestroyTextureView(colorTargetView);

    for (u32 i = 0; i < 2; i++)
    {
        Graphics::DestroyBuffer(raysBuffer[i]);
        Graphics::DestroyBuffer(rayCountBuffer[i]);
        Graphics::DestroyBuffer(pixelMappingBuffer[i]);
    }
    
    Graphics::DestroyBuffer(identityPixelMappingBuffer);
    Graphics::DestroyBuffer(shadowRayOriginsBuffer);
    Graphics::DestroyBuffer(shadowRayCountBuffer);
    Graphics::DestroyBuffer(shadowRayPixelMappingBuffer);
    Graphics::DestroyBuffer(intersectionsBuffer);
    Graphics::DestroyBuffer(payloadsBuffer);
    Graphics::DestroyBuffer(indirectArgsBuffer);
    Graphics::DestroyBuffer(raygenConstantsBuffer);
    Graphics::DestroyBuffer(resolveConstantsBuffer);

    Graphics::DestroyBindGroup(connectBindGroup);
    Graphics::DestroyBindGroup(raygenBindGroup);
    Graphics::DestroyBindGroup(resolveBindGroup);
}

void WfptPass::SetTlas(TlasHandle tlas)
{
    createInfo.tlas = tlas;

    BindGroupCreateInfo connectBindGroupCreateInfo{};
    connectBindGroupCreateInfo.name = "Wfpt Connect Bind Group";
    connectBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ConnectPipeline::Get(), 0);
    connectBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(shadowRayOriginsBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(shadowRayCountBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(payloadsBuffer)),
        BindGroupEntry(4, BindingResource::Buffer(shadowRayPixelMappingBuffer)),
        BindGroupEntry(5, BindingResource::AccelerationStructure(createInfo.tlas)),
    };
    Graphics::DestroyBindGroup(connectBindGroup);
    connectBindGroup = Graphics::CreateBindGroup(connectBindGroupCreateInfo);
}

void WfptPass::Dispatch(const Camera& camera, const BlasDataPool& blasDataPool, const MaterialPool& materialPool, const Sky& sky)
{
    RaygenConstants raygenConstants{};
    raygenConstants.invView = camera.GetInvView();
    raygenConstants.invProj = camera.GetInvProjection();
    raygenConstants.width = width;
    raygenConstants.height = height;
    Graphics::WriteBuffer(raygenConstantsBuffer, 0, &raygenConstants);

    ResolveConstants resolveConstants{};
    resolveConstants.width = width;
    resolveConstants.height = height;
    resolveConstants.sampleNumber = accumulationFrameIdx;
    Graphics::WriteBuffer(resolveConstantsBuffer, 0, &resolveConstants);

    u32 rayCountData = width * height;
    Graphics::WriteBuffer(rayCountBuffer[0], 0, &rayCountData);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Wfpt Raygen";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(RaygenPipeline::Get());
        Graphics::SetBindGroup(0, raygenBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    gbufferPass->Dispatch(camera);

    for (u32 bounce = 0; bounce < 1/*maxBounces*/; bounce++)
    {
        BufferHandle rays = raysBuffer[bounce % 2];
        BufferHandle outRays = raysBuffer[(bounce + 1) % 2];
        BufferHandle rayCount = rayCountBuffer[bounce % 2];
        BufferHandle outRayCount = rayCountBuffer[(bounce + 1) % 2];
        BufferHandle pixelMapping = bounce == 0 ? identityPixelMappingBuffer : pixelMappingBuffer[bounce % 2];
        BufferHandle outPixelMapping = pixelMappingBuffer[(bounce + 1) % 2];

        Graphics::ClearBuffer(outRayCount);
        Graphics::ClearBuffer(shadowRayCountBuffer);

        ShadeConstants shadeConstants{};
        shadeConstants.width = width;
        shadeConstants.height = height;
        shadeConstants.bounce = bounce;
        shadeConstants.seed = seed;
        shadeConstants.russianRoulette = russianRoulette;
        shadeConstants.hybrid = hybrid;

        BufferCreateInfo shadeConstantsCreateInfo{};
        shadeConstantsCreateInfo.name = "Wfpt Shade Constants Buffer";
        shadeConstantsCreateInfo.size = sizeof(ShadeConstants);
        shadeConstantsCreateInfo.data = &shadeConstants;
        shadeConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
        BufferHandle shadeConstantsBuffer = Graphics::CreateBuffer(shadeConstantsCreateInfo);

        BindGroupCreateInfo extendBindGroupCreateInfo{};
        extendBindGroupCreateInfo.name = "Wfpt Extend Bind Group";
        extendBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ExtendPipeline::Get(), 0);
        extendBindGroupCreateInfo.entries = {
            BindGroupEntry(0, BindingResource::Buffer(rays)),
            BindGroupEntry(1, BindingResource::Buffer(rayCount)),
            BindGroupEntry(2, BindingResource::Buffer(intersectionsBuffer)),
            BindGroupEntry(3, BindingResource::Buffer(pixelMapping)),
            BindGroupEntry(4, BindingResource::AccelerationStructure(createInfo.tlas))
        };
        BindGroupHandle extendBindGroup = Graphics::CreateBindGroup(extendBindGroupCreateInfo);
    
        BindGroupCreateInfo shadeBindGroupCreateInfo{};
        shadeBindGroupCreateInfo.name = "Wfpt Shade Bind Group";
        shadeBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ShadePipeline::Get(), 0);
        shadeBindGroupCreateInfo.entries = {
            BindGroupEntry(0, BindingResource::Buffer(shadeConstantsBuffer)),
            BindGroupEntry(1, BindingResource::Buffer(rays)),
            BindGroupEntry(2, BindingResource::Buffer(outRays)),
            BindGroupEntry(3, BindingResource::Buffer(rayCount)),
            BindGroupEntry(4, BindingResource::Buffer(outRayCount)),
            BindGroupEntry(5, BindingResource::Buffer(pixelMapping)),
            BindGroupEntry(6, BindingResource::Buffer(outPixelMapping)),
            BindGroupEntry(7, BindingResource::Buffer(payloadsBuffer)),
            BindGroupEntry(8, BindingResource::Buffer(intersectionsBuffer)),
            BindGroupEntry(9, BindingResource::Buffer(shadowRayOriginsBuffer)),
            BindGroupEntry(11, BindingResource::Buffer(shadowRayCountBuffer)),
            BindGroupEntry(12, BindingResource::Buffer(shadowRayPixelMappingBuffer)),
            BindGroupEntry(13, BindingResource::TextureView(gbufferPass->GetColorTargetView()))
        };
        BindGroupHandle shadeBindGroup = Graphics::CreateBindGroup(shadeBindGroupCreateInfo);
        BindGroupHandle shadeBlasDataPoolGroup = blasDataPool.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle shadeMaterialPoolGroup = materialPool.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle shadeSkyGroup = sky.CreateBindGroup(ShadePipeline::Get());
        BindGroupHandle shadeRestirGroup = restirDiPass->CreateBindGroup(ShadePipeline::Get(), false);
        
        BindGroupHandle connectBlasDataPoolGroup = blasDataPool.CreateBindGroup(ConnectPipeline::Get());
        BindGroupHandle connectSkyGroup = sky.CreateBindGroup(ConnectPipeline::Get());
        BindGroupHandle connectRestirGroup = restirDiPass->CreateBindGroup(ConnectPipeline::Get(), true);// RestirDiPass::SPATIAL_REUSE_PASSES % 2 == 0);

        WriteIndirectArgsPass writeIndirectArgs(128);
        writeIndirectArgs.Dispatch(indirectArgsBuffer, rayCount);

        if (bounce > 0 || !hybrid)
        {
            computePassDescriptor.name = "Wfpt Extend";
            computePass = Graphics::BeginComputePass(computePassDescriptor);
            {
                Graphics::SetComputePipeline(ExtendPipeline::Get());
                Graphics::SetBindGroup(0, extendBindGroup);
                Graphics::DispatchWorkgroupsIndirect(indirectArgsBuffer);
            }
            Graphics::EndComputePass(computePass);
        }

        computePassDescriptor.name = "Wfpt Shade";
        computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(ShadePipeline::Get());
            Graphics::SetBindGroup(0, shadeBindGroup);
            Graphics::SetBindGroup(BlasDataPool::BIND_GROUP_SET, shadeBlasDataPoolGroup);
            Graphics::SetBindGroup(MaterialPool::BIND_GROUP_SET, shadeMaterialPoolGroup);
            Graphics::SetBindGroup(Sky::BIND_GROUP_SET, shadeSkyGroup);
            Graphics::SetBindGroup(Restir::BIND_GROUP_SET, shadeRestirGroup);
            Graphics::DispatchWorkgroupsIndirect(indirectArgsBuffer);
        }
        Graphics::EndComputePass(computePass);

        restirDiPass->seed = seed;
        restirDiPass->unbiased = unbiased;
        restirDiPass->jacobian = jacobian;
        restirDiPass->Dispatch(camera, createInfo.tlas, gbufferPass->GetColorTargetView(), gbufferPass->GetColorTargetHistoryView(), blasDataPool, sky);
        
        writeIndirectArgs.Dispatch(indirectArgsBuffer, shadowRayCountBuffer);

        computePassDescriptor.name = "Wfpt Connect";
        computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(ConnectPipeline::Get());
            Graphics::SetBindGroup(0, connectBindGroup);
            Graphics::SetBindGroup(BlasDataPool::BIND_GROUP_SET, connectBlasDataPoolGroup);
            Graphics::SetBindGroup(Sky::BIND_GROUP_SET, connectSkyGroup);
            Graphics::SetBindGroup(Restir::BIND_GROUP_SET, connectRestirGroup);
            Graphics::DispatchWorkgroupsIndirect(indirectArgsBuffer);
        }
        Graphics::EndComputePass(computePass);

        Graphics::DestroyBindGroup(extendBindGroup);
        Graphics::DestroyBindGroup(shadeBindGroup);
        Graphics::DestroyBindGroup(shadeBlasDataPoolGroup);
        Graphics::DestroyBindGroup(shadeMaterialPoolGroup);
        Graphics::DestroyBindGroup(shadeSkyGroup);
        Graphics::DestroyBindGroup(shadeRestirGroup);
        Graphics::DestroyBindGroup(connectRestirGroup);
        Graphics::DestroyBuffer(shadeConstantsBuffer);
    }

    computePassDescriptor.name = "Wfpt Resolve";
    computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(ResolvePipeline::Get());
        Graphics::SetBindGroup(0, resolveBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    gbufferPass->NextFrame();
}

void WfptPass::ClearPipelineCache()
{
    ConnectPipeline::Clear();
    ExtendPipeline::Clear();
    RaygenPipeline::Clear();
    ResolvePipeline::Clear();
    ShadePipeline::Clear();
}