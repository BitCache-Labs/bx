#include "bx/framework/systems/renderer/material_pool.hpp"

#include "bx/framework/resources/material.hpp"

MaterialPool::MaterialPool()
    : textures{}
{
    BufferCreateInfo materialDescriptorsCreateInfo{};
    materialDescriptorsCreateInfo.name = "Material Descriptors Buffer";
    materialDescriptorsCreateInfo.size = MAX_MATERIALS * sizeof(MaterialDescriptor);
    materialDescriptorsCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST;
    materialDescriptorsBuffer = Graphics::CreateBuffer(materialDescriptorsCreateInfo);

    SamplerCreateInfo linearRepeatCreateInfo{};
    linearRepeatCreateInfo.name = "Nert Linear Repeat Sampler";
    linearRepeatCreateInfo.addressModeU = SamplerAddressMode::REPEAT;
    linearRepeatCreateInfo.addressModeV = SamplerAddressMode::REPEAT;
    linearRepeatCreateInfo.addressModeW = SamplerAddressMode::REPEAT;
    linearRepeatCreateInfo.minFilter = FilterMode::LINEAR;
    linearRepeatCreateInfo.magFilter = FilterMode::LINEAR;
    sampler = Graphics::CreateSampler(linearRepeatCreateInfo);
}

MaterialPool::~MaterialPool()
{
    Graphics::DestroyBuffer(materialDescriptorsBuffer);
    Graphics::DestroySampler(sampler);
}

u32 MaterialPool::AvailableTextureIdx() const
{
    for (u32 i = 0; i < MAX_TEXTURES; i++)
    {
        if (!textures[i])
        {
            return i;
        }
    }
    
    BX_FAIL("No more available textures.");
    return 0;
}

MaterialPool::MaterialDescriptor MaterialPool::UpdateMaterial(const Material& material, u32 descriptorIdx)
{
    MaterialDescriptor materialDescriptor{};
    materialDescriptor.baseColorFactor = material.GetBaseColorFactor().Xyz();
    
    const auto& materialTextures = material.GetTextures();

    auto baseColorTextureIter = materialTextures.find("Albedo");
    if (baseColorTextureIter != materialTextures.end())
    {
        u32 textureIdx = AvailableTextureIdx();
        textures[textureIdx] = baseColorTextureIter->second.GetData().GetTexture();
        materialDescriptor.baseColorTexture = textureIdx;
    }

    materialDescriptor.isMirror = material.IsMirror();

    if (material.IsEmissive())
    {
        materialDescriptor.emissiveFactor = material.GetEmissiveFactor() * 120.0;
    }

    Graphics::WriteBuffer(materialDescriptorsBuffer, descriptorIdx * sizeof(MaterialDescriptor), &materialDescriptor, sizeof(MaterialDescriptor));

    return materialDescriptor;
}

u32 MaterialPool::SubmitInstance(const Material& material, ResourceHandle resourceHandle)
{
    u32 materialIdx;
    auto descriptorIndexIter = materialDescriptorIndices.find(resourceHandle);
    if (descriptorIndexIter == materialDescriptorIndices.end())
    {
        materialIdx = materialDescriptors.size();

        MaterialDescriptor descriptor = UpdateMaterial(material, materialDescriptorCount++);
        materialDescriptors.push_back(descriptor);
        materialDescriptorIndices.insert(std::make_pair(resourceHandle, materialIdx));
    }
    else
    {
        materialIdx = descriptorIndexIter->second;

        if (material.m_graphicsDirty)
        {
            material.m_graphicsDirty = false;

            MaterialDescriptor descriptor = UpdateMaterial(material, materialIdx);
            materialDescriptors[materialIdx] = descriptor;
        }
    }

    return materialIdx;
}

void MaterialPool::Submit()
{
    
}

BindGroupLayoutDescriptor MaterialPool::GetBindGroupLayout()
{
    return BindGroupLayoutDescriptor(BIND_GROUP_SET, {
            BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),                                                     // materialDescriptors
            BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT), Optional<u32>::Some(MAX_TEXTURES)),    // materialTextures
            BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::Sampler()),                                                               // materialSampler
        });
}

BindGroupHandle MaterialPool::CreateBindGroup(ComputePipelineHandle pipeline) const
{
    List<TextureViewHandle> textureViews(MAX_TEXTURES);
    for (u32 i = 0; i < MAX_TEXTURES; i++)
    {
        if (textures[i])
        {
            textureViews[i] = Graphics::CreateTextureView(textures[i]);
        }
        else
        {
            textureViews[i] = Graphics::EmptyTextureView();
        }
    }

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Material Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(pipeline, BIND_GROUP_SET);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(materialDescriptorsBuffer)),
        BindGroupEntry(1, BindingResource::TextureViewArray(textureViews)),
        BindGroupEntry(2, BindingResource::Sampler(sampler)),
    };

    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

    for (u32 i = 0; i < MAX_TEXTURES; i++)
    {
        if (textures[i])
        {
            Graphics::DestroyTextureView(textureViews[i]);
        }
    }

    return bindGroup;
}