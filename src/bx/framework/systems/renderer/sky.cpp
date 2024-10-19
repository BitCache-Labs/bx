#include "bx/framework/systems/renderer/sky.hpp"

struct SkyConstants
{
    Vec3 direction = Vec3(-0.3, -1.0, 0.0);
    f32 size = 0.5;
    Vec3 color = Vec3::One();
    f32 intensity = 50.0;
};

Sky::Sky()
{
    BufferCreateInfo skyConstantsCreateInfo{};
    skyConstantsCreateInfo.name = "Sky Constants Buffer";
    skyConstantsCreateInfo.size = sizeof(SkyConstants);
    skyConstantsCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    skyConstantsBuffer = Graphics::CreateBuffer(skyConstantsCreateInfo);

    SamplerCreateInfo linearRepeatCreateInfo{};
    linearRepeatCreateInfo.name = "Sky Linear Repeat Sampler";
    linearRepeatCreateInfo.addressModeU = SamplerAddressMode::REPEAT;
    linearRepeatCreateInfo.addressModeV = SamplerAddressMode::REPEAT;
    linearRepeatCreateInfo.addressModeW = SamplerAddressMode::REPEAT;
    linearRepeatCreateInfo.minFilter = FilterMode::LINEAR;
    linearRepeatCreateInfo.magFilter = FilterMode::LINEAR;
    sampler = Graphics::CreateSampler(linearRepeatCreateInfo);
}

Sky::~Sky()
{
    Graphics::DestroyBuffer(skyConstantsBuffer);

    if (skyTextureView.IsSome())
    {
        Graphics::DestroyTextureView(skyTextureView.Unwrap());
    }
    Graphics::DestroySampler(sampler);
}

void Sky::SetSkyTexture(const Resource<Texture>& texture)
{
    if (skyTextureView.IsSome())
    {
        Graphics::DestroyTextureView(skyTextureView.Unwrap());
    }

    skyTextureResource = Optional<Resource<Texture>>::Some(texture);
    skyTextureView = Optional<TextureViewHandle>::Some(Graphics::CreateTextureView(texture->GetTexture()));
}

void Sky::Submit()
{
    SkyConstants skyConstants{};
    skyConstants.direction = sunInfo.direction.Normalized();
    skyConstants.size = sunInfo.size;
    skyConstants.color = sunInfo.color.Xyz();
    skyConstants.intensity = sunInfo.intensity;
    Graphics::WriteBuffer(skyConstantsBuffer, 0, &skyConstants);
}

BindGroupLayoutDescriptor Sky::GetBindGroupLayout()
{
    return BindGroupLayoutDescriptor(BIND_GROUP_SET, {
            BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()), // skyConstants
            BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)), // skyImage
            BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler()), // skySampler
        });
}

BindGroupHandle Sky::CreateBindGroup(ComputePipelineHandle pipeline) const
{
    BindGroupCreateInfo createInfo{};
    createInfo.name = "Sky Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(pipeline, BIND_GROUP_SET);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(skyConstantsBuffer)),
        BindGroupEntry(1, BindingResource::TextureView(skyTextureView.IsSome() ? skyTextureView.Unwrap() : Graphics::EmptyTextureView())),
        BindGroupEntry(2, BindingResource::Sampler(sampler))
    };
    return Graphics::CreateBindGroup(createInfo);
}