#include "bx/framework/systems/renderer/reprojection_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

struct ReprojectionConstants
{
    Mat4 clipToView;
    Mat4 viewToClip;
    Mat4 clipToPrevClip;
    u32 width;
    u32 height;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct ReprojectionPipeline : public LazyInit<ReprojectionPipeline, ComputePipelineHandle>
{
    ReprojectionPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Reprojection Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/reprojection/reprojection.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RG32_FLOAT)),
                BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler()),
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Reprojection Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};

template<>
std::unique_ptr<ReprojectionPipeline> LazyInit<ReprojectionPipeline, ComputePipelineHandle>::cache = nullptr;

ReprojectionPass::ReprojectionPass(u32 width, u32 height)
    : width(width), height(height)
{
    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "Reprojection Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(ReprojectionConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);

    TextureCreateInfo reprojectionCreateInfo{};
    reprojectionCreateInfo.name = "Reprojection Texture";
    reprojectionCreateInfo.size = Extend3D(width, height, 1);
    reprojectionCreateInfo.format = TextureFormat::RG32_FLOAT;
    reprojectionCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::TEXTURE_BINDING;
    reprojectionTexture = Graphics::CreateTexture(reprojectionCreateInfo);
    reprojectionTextureView = Graphics::CreateTextureView(reprojectionTexture);

    SamplerCreateInfo pointClampCreateInfo{};
    pointClampCreateInfo.name = "Reprojection Nearest Clamp Sampler";
    pointClampCreateInfo.addressModeU = SamplerAddressMode::CLAMP_TO_EDGE;
    pointClampCreateInfo.addressModeV = SamplerAddressMode::CLAMP_TO_EDGE;
    pointClampCreateInfo.addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
    pointClampCreateInfo.minFilter = FilterMode::NEAREST;
    pointClampCreateInfo.magFilter = FilterMode::NEAREST;
    nearestClampSampler = Graphics::CreateSampler(pointClampCreateInfo);
}

ReprojectionPass::~ReprojectionPass()
{
    Graphics::DestroyBuffer(constantBuffer);
    Graphics::DestroyTextureView(reprojectionTextureView);
    Graphics::DestroyTexture(reprojectionTexture);
    Graphics::DestroySampler(nearestClampSampler);
}

TextureViewHandle ReprojectionPass::GetReprojectionView() const
{
    return reprojectionTextureView;
}

void ReprojectionPass::Dispatch(const Camera& camera, TextureViewHandle depthView, TextureViewHandle velocityView)
{
    Mat4 clipToPrevClip = camera.GetPrevProjection()
        * camera.GetPrevView()
        * camera.GetInvView()
        * camera.GetInvProjection();

    ReprojectionConstants constants{};
    constants.clipToView = camera.GetInvProjection();
    constants.viewToClip = camera.GetProjection();
    constants.clipToPrevClip = clipToPrevClip;
    constants.width = width;
    constants.height = height;
    Graphics::WriteBuffer(constantBuffer, 0, &constants, sizeof(ReprojectionConstants));

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reprojection BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(ReprojectionPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(constantBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(velocityView)),
        BindGroupEntry(2, BindingResource::TextureView(depthView)),
        BindGroupEntry(3, BindingResource::TextureView(reprojectionTextureView)),
        BindGroupEntry(4, BindingResource::Sampler(nearestClampSampler)),
    };
    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Reprojection";
    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(ReprojectionPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(Math::DivCeil(width, 16), Math::DivCeil(height, 16), 1);
    }
    Graphics::EndComputePass(computePass);

    Graphics::DestroyBindGroup(bindGroup);
}

void ReprojectionPass::ClearPipelineCache()
{
    ReprojectionPipeline::Clear();
}