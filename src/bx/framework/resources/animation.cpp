#include "bx/framework/resources/animation.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/modules/graphics.hpp>

#include "bx/framework/resources/animation.serial.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
#include <fstream>
#include <sstream>

template<>
bool Resource<Animation>::Save(const String& filename, const Animation& data)
{
    // Serialize data
    std::ofstream stream(File::GetPath(filename), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryOutputArchive archive(stream);
    archive(cereal::make_nvp("animation", data));

    return true;
}

template<>
bool Resource<Animation>::Load(const String& filename, Animation& data)
{
    // Deserialize data
    std::ifstream stream(File::GetPath(filename), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryInputArchive archive(stream);
    archive(cereal::make_nvp("animation", data));

    return true;
}

template<>
void Resource<Animation>::Unload(Animation& data)
{
}