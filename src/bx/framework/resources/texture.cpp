#include "bx/framework/resources/texture.hpp"
#include "bx/framework/resources/texture.serial.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/modules/graphics.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
#include <fstream>
#include <sstream>

template<>
bool Resource<Texture>::Save(const String& filename, const Texture& data)
{
    // TODO: Use astc compression: https://developer.nvidia.com/astc-texture-compression-for-game-assets
    // Possibly using this lib: https://github.com/ARM-software/astc-encoder
    // @Conor, I propose we use bc encoding as it has a wider range of supported devices
    // On top of that, bc7 performs a bit better

    std::ofstream stream(File::GetPath(filename), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryOutputArchive archive(stream);
    archive(cereal::make_nvp("texture", data));

    return true;
}

template<>
bool Resource<Texture>::Load(const String& filename, Texture& data)
{
    // Deserialize data
    std::ifstream stream(File::GetPath(filename), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryInputArchive archive(stream);
    archive(cereal::make_nvp("texture", data));

    // TODO: we NEED texture types to distinguish between srgb, unorm and float unorm textures!!!!!!

    TextureCreateInfo createInfo{};
    createInfo.name = filename;
    createInfo.format = TextureFormat::RGBA8_UNORM_SRGB;
    createInfo.size = Extend3D(data.width, data.height, 1);
    createInfo.usageFlags = TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::COPY_SRC;
    createInfo.data = static_cast<const void*>(data.pixels.data());
    data.m_texture = Graphics::CreateTexture(createInfo);

    return true;
}

template<>
void Resource<Texture>::Unload(Texture& data)
{
    if (data.m_texture)
    {
        Graphics::DestroyTexture(data.m_texture);
    }
}