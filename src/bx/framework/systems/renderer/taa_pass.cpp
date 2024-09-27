#include "bx/framework/systems/renderer/taa_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/framework/components/transform.hpp"
#include "bx/framework/components/mesh_filter.hpp"
#include "bx/framework/components/mesh_renderer.hpp"

struct TaaConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct TaaPipeline : public LazyInit<TaaPipeline, ComputePipelineHandle>
{
    TaaPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "TAA Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/taa/taa.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(6, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT))
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "TAA Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};

template<>
std::unique_ptr<TaaPipeline> LazyInit<TaaPipeline, ComputePipelineHandle>::cache = nullptr;

TaaPass::TaaPass(u32 width, u32 height)
    : width(width), height(height)
{
    TextureCreateInfo resolvedColorTargetCreateInfo{};
    resolvedColorTargetCreateInfo.name = "TAA Resolved Color Target";
    resolvedColorTargetCreateInfo.size = Extend3D(width, height, 1);
    resolvedColorTargetCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    resolvedColorTargetCreateInfo.usageFlags = TextureUsageFlags::RENDER_ATTACHMENT | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::STORAGE_BINDING;
    resolvedColorTarget = Graphics::CreateTexture(resolvedColorTargetCreateInfo);
    resolvedColorTargetView = Graphics::CreateTextureView(resolvedColorTarget);

    resolvedColorTargetCreateInfo.name = "TAA Resolved Color Target History";
    resolvedColorTargetHistory = Graphics::CreateTexture(resolvedColorTargetCreateInfo);
    resolvedColorTargetHistoryView = Graphics::CreateTextureView(resolvedColorTarget);

    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "TAA Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(TaaConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);
}

TaaPass::~TaaPass()
{
    Graphics::DestroyTextureView(resolvedColorTargetView);
    Graphics::DestroyTexture(resolvedColorTarget);
    Graphics::DestroyTextureView(resolvedColorTargetHistoryView);
    Graphics::DestroyTexture(resolvedColorTargetHistory);
    Graphics::DestroyBuffer(constantBuffer);
}

TextureHandle TaaPass::GetResolvedColorTarget() const
{
    return resolvedColorTarget;
}

void TaaPass::Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle gbufferHistoryView, TextureViewHandle velocityTargetView)
{
    TaaConstants constants{};
    constants.width = width;
    constants.height = height;
    Graphics::WriteBuffer(constantBuffer, 0, &constants, sizeof(TaaConstants));

    TextureViewHandle colorTargetView = Graphics::CreateTextureView(colorTarget);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "TAA BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(TaaPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(constantBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(colorTargetView)),
        BindGroupEntry(2, BindingResource::TextureView(resolvedColorTargetHistoryView)),
        BindGroupEntry(3, BindingResource::TextureView(velocityTargetView)),
        BindGroupEntry(4, BindingResource::TextureView(gbufferView)),
        BindGroupEntry(5, BindingResource::TextureView(gbufferHistoryView)),
        BindGroupEntry(6, BindingResource::TextureView(resolvedColorTargetView)),
    };
    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "TAA";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(TaaPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::CopyTexture(resolvedColorTarget, resolvedColorTargetHistory);

    Graphics::DestroyTextureView(colorTargetView);
    Graphics::DestroyBindGroup(bindGroup);
}

void TaaPass::ClearPipelineCache()
{
    TaaPipeline::Clear();
}