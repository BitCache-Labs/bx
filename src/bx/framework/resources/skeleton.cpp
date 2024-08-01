#include "bx/framework/resources/skeleton.hpp"
#include "bx/framework/resources/skeleton.serial.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/modules/graphics.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
#include <fstream>
#include <sstream>

template<>
bool Resource<Skeleton>::Save(const String& filename, const Skeleton& data)
{
    // Serialize data
    std::ofstream stream(File::GetPath(filename), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryOutputArchive archive(stream);
    archive(cereal::make_nvp("skeleton", data));

    return true;
}

template<>
bool Resource<Skeleton>::Load(const String& filename, Skeleton& data)
{
    // Deserialize data
    std::ifstream stream(File::GetPath(filename), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryInputArchive archive(stream);
    archive(cereal::make_nvp("skeleton", data));

    return true;
}

template<>
void Resource<Skeleton>::Unload(Skeleton& data)
{
}