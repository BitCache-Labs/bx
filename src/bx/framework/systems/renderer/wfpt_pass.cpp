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
};

struct ShadeConstants
{
    u32 width;
    u32 height;
};

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
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::AccelerationStructure()),
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
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false)),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false))
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
{
    colorTargetView = Graphics::CreateTextureView(createInfo.colorTarget);

    const TextureCreateInfo& colorTargetCreateInfo = Graphics::GetTextureCreateInfo(createInfo.colorTarget);
    BX_ASSERT(colorTargetCreateInfo.format == TextureFormat::RGBA32_FLOAT, "Wfpt color target must be in TextureFormat::RGBA32_FLOAT.");
    width = colorTargetCreateInfo.size.width;
    height = colorTargetCreateInfo.size.height;

    BufferCreateInfo raysCreateInfo{};
    raysCreateInfo.name = "Wfpt Rays Buffer";
    raysCreateInfo.size = width * height * sizeof(Ray);
    raysCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    raysBuffer = Graphics::CreateBuffer(raysCreateInfo);

    BufferCreateInfo rayCountCreateInfo{};
    rayCountCreateInfo.name = "Wfpt Ray Count Buffer";
    rayCountCreateInfo.size = sizeof(u32);
    rayCountCreateInfo.usageFlags = BufferUsageFlags::STORAGE;
    rayCountBuffer = Graphics::CreateBuffer(rayCountCreateInfo);

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

    BufferCreateInfo shadeConstantsCreateInfo{};
    shadeConstantsCreateInfo.name = "Wfpt Shade Constants Buffer";
    shadeConstantsCreateInfo.size = sizeof(ShadeConstants);
    shadeConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    shadeConstantsBuffer = Graphics::CreateBuffer(shadeConstantsCreateInfo);

    BindGroupCreateInfo extendBindGroupCreateInfo{};
    extendBindGroupCreateInfo.name = "Wfpt Extend Bind Group";
    extendBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ExtendPipeline::Get(), 0);
    extendBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(raysBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(rayCountBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(intersectionsBuffer)),
        BindGroupEntry(3, BindingResource::AccelerationStructure(createInfo.tlas))
    };
    extendBindGroup = Graphics::CreateBindGroup(extendBindGroupCreateInfo);

    BindGroupCreateInfo raygenBindGroupCreateInfo{};
    raygenBindGroupCreateInfo.name = "Wfpt Raygen Bind Group";
    raygenBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(RaygenPipeline::Get(), 0);
    raygenBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(raygenConstantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(raysBuffer))
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

    BindGroupCreateInfo shadeBindGroupCreateInfo{};
    shadeBindGroupCreateInfo.name = "Wfpt Shade Bind Group";
    shadeBindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(ShadePipeline::Get(), 0);
    shadeBindGroupCreateInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(shadeConstantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(raysBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(rayCountBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(intersectionsBuffer)),
        BindGroupEntry(4, BindingResource::Buffer(payloadsBuffer))
    };
    shadeBindGroup = Graphics::CreateBindGroup(shadeBindGroupCreateInfo);
}

WfptPass::~WfptPass()
{
    Graphics::DestroyTextureView(colorTargetView);

    Graphics::DestroyBuffer(raysBuffer);
    Graphics::DestroyBuffer(rayCountBuffer);
    Graphics::DestroyBuffer(intersectionsBuffer);
    Graphics::DestroyBuffer(payloadsBuffer);
    Graphics::DestroyBuffer(raygenConstantsBuffer);
    Graphics::DestroyBuffer(resolveConstantsBuffer);
    Graphics::DestroyBuffer(shadeConstantsBuffer);

    Graphics::DestroyBindGroup(extendBindGroup);
    Graphics::DestroyBindGroup(raygenBindGroup);
    Graphics::DestroyBindGroup(resolveBindGroup);
    Graphics::DestroyBindGroup(shadeBindGroup);
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

    ShadeConstants shadeConstants{};
    shadeConstants.width = width;
    shadeConstants.height = height;
    Graphics::WriteBuffer(shadeConstantsBuffer, 0, &shadeConstants);

    ResolveConstants resolveConstants{};
    resolveConstants.width = width;
    resolveConstants.height = height;
    Graphics::WriteBuffer(resolveConstantsBuffer, 0, &resolveConstants);

    u32 rayCount = width * height;
    Graphics::WriteBuffer(rayCountBuffer, 0, &rayCount);

    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(RaygenPipeline::Get());
        Graphics::SetBindGroup(0, raygenBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);

        Graphics::SetComputePipeline(ExtendPipeline::Get());
        Graphics::SetBindGroup(0, extendBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);

        Graphics::SetComputePipeline(ShadePipeline::Get());
        Graphics::SetBindGroup(0, shadeBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width * height, 128), 1, 1);

        Graphics::SetComputePipeline(ResolvePipeline::Get());
        Graphics::SetBindGroup(0, resolveBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);
}

void WfptPass::ClearPipelineCache()
{
    ExtendPipeline::Clear();
    RaygenPipeline::Clear();
    ShadePipeline::Clear();
}