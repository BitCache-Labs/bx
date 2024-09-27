#include "bx/framework/systems/renderer/reblur_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/engine/core/file.hpp"

struct PreBlurConstants
{
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct PreBlurPipeline : public LazyInit<PreBlurPipeline, ComputePipelineHandle>
{
    PreBlurPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Reblur Pre Blur Reuse Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/reblur/pre_blur.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Reblur Pre Blur Reuse Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<PreBlurPipeline> LazyInit<PreBlurPipeline, ComputePipelineHandle>::cache = nullptr;

ReblurPass::ReblurPass(u32 width, u32 height)
    : width(width), height(height)
{
    BufferCreateInfo preBlurConstantsCreateInfo{};
    preBlurConstantsCreateInfo.name = "Reblur Temporal Reuse Constants Buffer";
    preBlurConstantsCreateInfo.size = sizeof(PreBlurConstants);
    preBlurConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    preBlurConstantsBuffer = Graphics::CreateBuffer(preBlurConstantsCreateInfo);

    TextureCreateInfo tmpIlluminationCreateInfo{};
    tmpIlluminationCreateInfo.name = "Reblur Temp Illumination Texture";
    tmpIlluminationCreateInfo.size = Extend3D(width, height, 1);
    tmpIlluminationCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    tmpIlluminationCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::COPY_SRC;
    tmpIlluminationTexture = Graphics::CreateTexture(tmpIlluminationCreateInfo);
    tmpIlluminationTextureView = Graphics::CreateTextureView(tmpIlluminationTexture);
}

ReblurPass::~ReblurPass()
{
    Graphics::DestroyBuffer(preBlurConstantsBuffer);

    Graphics::DestroyTextureView(tmpIlluminationTextureView);
    Graphics::DestroyTexture(tmpIlluminationTexture);
}

BindGroupHandle ReblurPass::CreatePreBlurBindGroup(const ReblurDispatchInfo& dispatchInfo) const
{
    TextureViewHandle unresolvedIlluminationView = Graphics::CreateTextureView(dispatchInfo.unresolvedIllumination);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reblur Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(PreBlurPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(preBlurConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(unresolvedIlluminationView)),
        BindGroupEntry(2, BindingResource::TextureView(tmpIlluminationTextureView))
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);

    return bindGroup;
}

void ReblurPass::Dispatch(const ReblurDispatchInfo& dispatchInfo)
{
    BindGroupHandle preBlurBindGroup = CreatePreBlurBindGroup(dispatchInfo);

    PreBlurConstants preBlurConstants{};
    preBlurConstants.width = width;
    preBlurConstants.height = height;
    Graphics::WriteBuffer(preBlurConstantsBuffer, 0, &preBlurConstants);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Reblur Temporal Reuse";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(PreBlurPipeline::Get());
        Graphics::SetBindGroup(0, preBlurBindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::CopyTexture(tmpIlluminationTexture, dispatchInfo.unresolvedIllumination);

    Graphics::DestroyBindGroup(preBlurBindGroup);
}

void ReblurPass::ClearPipelineCache()
{
    PreBlurPipeline::Clear();
}