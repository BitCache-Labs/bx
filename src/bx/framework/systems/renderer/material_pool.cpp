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
}

MaterialPool::~MaterialPool()
{
    Graphics::DestroyBuffer(materialDescriptorsBuffer);
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

MaterialPool::MaterialDescriptor MaterialPool::AllocateMaterial(const Material& material)
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

    Graphics::WriteBuffer(materialDescriptorsBuffer, materialDescriptorCount * sizeof(MaterialDescriptor), &materialDescriptor, sizeof(MaterialDescriptor));
    materialDescriptorCount++;

    return materialDescriptor;
}

u32 MaterialPool::SubmitInstance(const Material& material, ResourceHandle resourceHandle)
{
    u32 materialIdx;
    auto descriptorIndexIter = materialDescriptorIndices.find(resourceHandle);
    if (descriptorIndexIter == materialDescriptorIndices.end())
    {
        materialIdx = materialDescriptors.size();

        MaterialDescriptor descriptor = AllocateMaterial(material);
        materialDescriptors.push_back(descriptor);
        materialDescriptorIndices.insert(std::make_pair(resourceHandle, materialIdx));
    }
    else
    {
        materialIdx = descriptorIndexIter->second;
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
        BindGroupEntry(1, BindingResource::TextureViewArray(textureViews))
    };
    return Graphics::CreateBindGroup(createInfo);
}