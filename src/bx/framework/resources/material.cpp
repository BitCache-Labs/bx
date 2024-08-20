#include "bx/framework/resources/material.hpp"
#include "bx/framework/resources/material.serial.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/modules/graphics.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
#include <fstream>
#include <sstream>

template<>
bool Resource<Material>::Save(const String& filename, const Material& data)
{
    // Serialize data
    std::ofstream stream(File::GetPath(filename));
    if (stream.fail())
        return false;

    cereal::JSONOutputArchive archive(stream);
    archive(cereal::make_nvp("material", data));

    return true;
}

template<>
bool Resource<Material>::Load(const String& filename, Material& data)
{
    // Deserialize data
    std::ifstream stream(File::GetPath(filename));
    if (stream.fail())
        return false;

    cereal::JSONInputArchive archive(stream);
    archive(cereal::make_nvp("material", data));

    return true;
}

template<>
void Resource<Material>::Unload(Material& data)
{
    for (auto& bindGroup : data.m_bindGroupCache)
    {
        Graphics::DestroyBindGroup(bindGroup.second);
    }
}

BindGroupHandle Material::GetBindGroup(BindGroupLayoutHandle layout) const
{
    auto bindGroupIter = m_bindGroupCache.find(layout);
    if (bindGroupIter != m_bindGroupCache.end())
    {
        return bindGroupIter->second;
    }
    else
    {
        auto albedoTextureIter = m_textures.find("Albedo");
        BX_ENSURE(albedoTextureIter != m_textures.end());
        TextureHandle albedoTexture = albedoTextureIter->second.GetData().GetTexture();
        TextureViewHandle albedoTextureView = Graphics::CreateTextureView(albedoTexture); // TODO: handle leak! don't care atm

        BindGroupCreateInfo createInfo{};
        createInfo.name = "Material Bind Group";
        createInfo.layout = layout;
        createInfo.entries = {
            // TODO: 3 is a bit weird, emulate bind GROUPS on opengl
            BindGroupEntry(3, BindingResource::TextureView(albedoTextureView))
        };

        BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);
        m_bindGroupCache.insert(std::make_pair(layout, bindGroup));
        return bindGroup;
    }
}

BindGroupLayoutDescriptor Material::GetBindGroupLayout()
{
    return BindGroupLayoutDescriptor(Material::SHADER_BIND_GROUP, {
        BindGroupLayoutEntry(3, ShaderStageFlags::FRAGMENT, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)) // layout (binding = 3) uniform sampler2D Albedo;
    });
}