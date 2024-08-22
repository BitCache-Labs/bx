#include "bx/framework/systems/renderer/wfpt_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

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

struct Intersection
{
    Vec2 uv;
    u32 primitiveIdx;
    u32 blasInstanceIdx;
    f32 t;
    b32 frontFace;
};

struct Payload
{
    Vec3 accumulated;
    u32 _PADDING0;
    Vec3 throughput;
    u32 rngState;
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
};

struct ShadeConstants
{
    u32 width;
    u32 height;
    u32 bounce;
    u32 seed;
};

struct ConnectPipeline : public LazyInit<ConnectPipeline, ComputePipelineHandle>
{
    ConnectPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Wfpt Connect Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/wfpt/connect.shader")));;
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // shadowRays
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // shadowRayDistances
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // shadowRayCount
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),    // payloads
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // shadowPixelMapping
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()), // scene
            })
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
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/wfpt/extend.shader")));;
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
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/wfpt/raygen.shader")));;
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
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/wfpt/resolve.shader")));;
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
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/wfpt/shade.shader")));;
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
                BindGroupLayoutEntry(9, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),    // shadowRays
                BindGroupLayoutEntry(10, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),   // shadowRayDistances
                BindGroupLayoutEntry(11, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),   // shadowRayCount
                BindGroupLayoutEntry(12, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),   // shadowPixelMapping
            })
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

    for (u32 i = 0; i < 2; i++)
    {
        BufferCreateInfo raysCreateInfo{};
        raysCreateInfo.name = Log::Format("Wfpt Rays {} Buffer", i);
        raysCreateInfo.size = width * height * sizeof(Ray);
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

    BufferCreateInfo shadowRaysCreateInfo{};
    shadowRaysCreateInfo.name = "Wfpt Shadow Rays Buffer";
    shadowRaysCreateInfo.size = width * height * sizeof(Ray);
    shadowRaysCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    shadowRaysBuffer = Graphics::CreateBuffer(shadowRaysCreateInfo);

    BufferCreateInfo shadowRayDistancesCreateInfo{};
    shadowRayDistancesCreateInfo.name = "Wfpt Shadow Ray Distances Buffer";
    shadowRayDistancesCreateInfo.size = width * height * sizeof(f32);
    shadowRayDistancesCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    shadowRayDistancesBuffer = Graphics::CreateBuffer(shadowRayDistancesCreateInfo);

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
        BindGroupEntry(0, BindingResource::Buffer(shadowRaysBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(shadowRayDistancesBuffer)),
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
}

WfptPass::~WfptPass()
{
    Graphics::DestroyTextureView(colorTargetView);

    for (u32 i = 0; i < 2; i++)
    {
        Graphics::DestroyBuffer(raysBuffer[i]);
        Graphics::DestroyBuffer(rayCountBuffer[i]);
        Graphics::DestroyBuffer(pixelMappingBuffer[i]);
    }
    
    Graphics::DestroyBuffer(shadowRaysBuffer);
    Graphics::DestroyBuffer(shadowRayDistancesBuffer);
    Graphics::DestroyBuffer(shadowRayCountBuffer);
    Graphics::DestroyBuffer(shadowRayPixelMappingBuffer);
    Graphics::DestroyBuffer(intersectionsBuffer);
    Graphics::DestroyBuffer(payloadsBuffer);
    Graphics::DestroyBuffer(raygenConstantsBuffer);
    Graphics::DestroyBuffer(resolveConstantsBuffer);

    Graphics::DestroyBindGroup(connectBindGroup);
    Graphics::DestroyBindGroup(raygenBindGroup);
    Graphics::DestroyBindGroup(resolveBindGroup);
}

void WfptPass::Dispatch(const Camera& camera)
{
    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Wavefront Path Tracer";

    RaygenConstants raygenConstants{};
    raygenConstants.invView = camera.GetInvView();
    raygenConstants.invProj = camera.GetInvProjection();
    raygenConstants.width = width;
    raygenConstants.height = height;
    Graphics::WriteBuffer(raygenConstantsBuffer, 0, &raygenConstants);

    ResolveConstants resolveConstants{};
    resolveConstants.width = width;
    resolveConstants.height = height;
    Graphics::WriteBuffer(resolveConstantsBuffer, 0, &resolveConstants);

    List<u32> pixelMappingData(width * height);
    for (u32 i = 0; i < width * height; i++)
    {
        pixelMappingData[i] = i;
    }
    Graphics::WriteBuffer(pixelMappingBuffer[0], 0, pixelMappingData.data());

    u32 rayCountData = width * height;
    Graphics::WriteBuffer(rayCountBuffer[0], 0, &rayCountData);

    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(RaygenPipeline::Get());
        Graphics::SetBindGroup(0, raygenBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);

        for (u32 bounce = 0; bounce < maxBounces; bounce++)
        {
            BufferHandle rays = raysBuffer[bounce % 2];
            BufferHandle outRays = raysBuffer[(bounce + 1) % 2];
            BufferHandle rayCount = rayCountBuffer[bounce % 2];
            BufferHandle outRayCount = rayCountBuffer[(bounce + 1) % 2];
            BufferHandle pixelMapping = pixelMappingBuffer[bounce % 2];
            BufferHandle outPixelMapping = pixelMappingBuffer[(bounce + 1) % 2];

            Graphics::ClearBuffer(outRayCount);
            Graphics::ClearBuffer(shadowRayCountBuffer);

            ShadeConstants shadeConstants{};
            shadeConstants.width = width;
            shadeConstants.height = height;
            shadeConstants.bounce = bounce;
            shadeConstants.seed = seed;

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
                BindGroupEntry(9, BindingResource::Buffer(shadowRaysBuffer)),
                BindGroupEntry(10, BindingResource::Buffer(shadowRayDistancesBuffer)),
                BindGroupEntry(11, BindingResource::Buffer(shadowRayCountBuffer)),
                BindGroupEntry(12, BindingResource::Buffer(shadowRayPixelMappingBuffer))
            };
            BindGroupHandle shadeBindGroup = Graphics::CreateBindGroup(shadeBindGroupCreateInfo);
            
            Graphics::SetComputePipeline(ExtendPipeline::Get());
            Graphics::SetBindGroup(0, extendBindGroup);
            Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);

            Graphics::SetComputePipeline(ShadePipeline::Get());
            Graphics::SetBindGroup(0, shadeBindGroup);
            Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);

            Graphics::SetComputePipeline(ConnectPipeline::Get());
            Graphics::SetBindGroup(0, connectBindGroup);
            Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);

            Graphics::DestroyBindGroup(extendBindGroup);
            Graphics::DestroyBindGroup(shadeBindGroup);
            Graphics::DestroyBuffer(shadeConstantsBuffer);
        }

        Graphics::SetComputePipeline(ResolvePipeline::Get());
        Graphics::SetBindGroup(0, resolveBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);
}

void WfptPass::ClearPipelineCache()
{
    ConnectPipeline::Clear();
    ExtendPipeline::Clear();
    RaygenPipeline::Clear();
    ResolvePipeline::Clear();
    ShadePipeline::Clear();
}