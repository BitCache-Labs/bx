#include "bx/framework/systems/renderer/reblur_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/engine/core/file.hpp"

struct ATrousConstants
{
    u32 globalWidth;
    u32 globalHeight;
    u32 width;
    u32 height;
    u32 stepSize;
    u32 _PADDING0;
    u32 _PADDING1;
    u32 _PADDING2;
};

struct ATrousPipeline : public LazyInit<ATrousPipeline, ComputePipelineHandle>
{
    ATrousPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Reblur A-Trous Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/reblur/a_trous.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),                                                             // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // inImage
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::READ, TextureFormat::RGBA32_FLOAT)),     // gbuffer
                BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT)),    // outImage
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Reblur A-Trous Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<ATrousPipeline> LazyInit<ATrousPipeline, ComputePipelineHandle>::cache = nullptr;

ReblurPass::ReblurPass(u32 width, u32 height, u32 lightingWidth, u32 lightingHeight)
    : width(width), height(height), lightingWidth(lightingWidth), lightingHeight(lightingHeight), frameIdx(0)
{
    SamplerCreateInfo linearClampCreateInfo{};
    linearClampCreateInfo.name = "Reblur Linear Clamp Sampler";
    linearClampCreateInfo.addressModeU = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.addressModeV = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
    linearClampCreateInfo.minFilter = FilterMode::LINEAR;
    linearClampCreateInfo.magFilter = FilterMode::LINEAR;
    linearClampSampler = Graphics::CreateSampler(linearClampCreateInfo);

    BufferCreateInfo preBlurConstantsCreateInfo{};
    preBlurConstantsCreateInfo.name = "Reblur Pre Blur Constants Buffer";
    preBlurConstantsCreateInfo.size = sizeof(ATrousConstants);
    preBlurConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    aTrousConstantsBuffer = Graphics::CreateBuffer(preBlurConstantsCreateInfo);

    TextureCreateInfo tmpIlluminationCreateInfo{};
    tmpIlluminationCreateInfo.name = "Reblur Temp Illumination Texture";
    tmpIlluminationCreateInfo.size = Extend3D(lightingWidth, lightingHeight, 1);
    tmpIlluminationCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    tmpIlluminationCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::COPY_SRC;
    tmpIlluminationTexture = Graphics::CreateTexture(tmpIlluminationCreateInfo);
    tmpIlluminationTextureView = Graphics::CreateTextureView(tmpIlluminationTexture);
}

ReblurPass::~ReblurPass()
{
    Graphics::DestroySampler(linearClampSampler);

    Graphics::DestroyBuffer(aTrousConstantsBuffer);

    Graphics::DestroyTextureView(tmpIlluminationTextureView);
    Graphics::DestroyTexture(tmpIlluminationTexture);
}

BindGroupHandle ReblurPass::CreateATrousBindGroup(const ReblurDispatchInfo& dispatchInfo, bool pingPong) const
{
    TextureViewHandle unresolvedIlluminationView = Graphics::CreateTextureView(dispatchInfo.unresolvedIllumination);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Reblur Pre Blur Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(ATrousPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(aTrousConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(pingPong ? tmpIlluminationTextureView : unresolvedIlluminationView)),
        BindGroupEntry(2, BindingResource::TextureView(dispatchInfo.gbufferView)),
        BindGroupEntry(3, BindingResource::TextureView(pingPong ? unresolvedIlluminationView : tmpIlluminationTextureView)),
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    Graphics::DestroyTextureView(unresolvedIlluminationView);

    return bindGroup;
}

void ReblurPass::Dispatch(const ReblurDispatchInfo& dispatchInfo)
{
    BindGroupHandle aTrousBindGroup[2];
    for (u32 i = 0; i < 2; i++)
    {
        aTrousBindGroup[i] = CreateATrousBindGroup(dispatchInfo, i % 2 != 0);
    }

    ATrousConstants aTrousConstants{};
    aTrousConstants.globalWidth = width;
    aTrousConstants.globalHeight = height;
    aTrousConstants.width = lightingWidth;
    aTrousConstants.height = lightingHeight;

    for (u32 i = 0; i < spatialFilterSteps; i++)
    {
        aTrousConstants.stepSize = 1 << i;
        Graphics::WriteBufferImmediate(aTrousConstantsBuffer, 0, &aTrousConstants);

        ComputePassDescriptor computePassDescriptor{};
        computePassDescriptor.name = "Reblur A-Trous";
        ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(ATrousPipeline::Get());
            Graphics::SetBindGroup(0, aTrousBindGroup[i % 2 == 0]);
            Graphics::DispatchWorkgroups(Math::DivCeil(lightingWidth, 16), Math::DivCeil(lightingHeight, 16), 1);
        }
        Graphics::EndComputePass(computePass);
    }

    if (spatialFilterSteps % 2 != 0)
    {
        Graphics::CopyTexture(tmpIlluminationTexture, dispatchInfo.unresolvedIllumination);
    }

    for (u32 i = 0; i < 2; i++)
    {
        Graphics::DestroyBindGroup(aTrousBindGroup[i]);
    }

    frameIdx++;
}

void ReblurPass::ClearPipelineCache()
{
    ATrousPipeline::Clear();
}