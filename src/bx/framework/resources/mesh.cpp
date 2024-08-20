#include "bx/framework/resources/mesh.hpp"
#include "bx/framework/resources/mesh.serial.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/modules/graphics.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
#include <fstream>
#include <sstream>

template<>
bool Resource<Mesh>::Save(const String& filename, const Mesh& data)
{
    // TODO: Use some sort of compression (research needs to be done)

    // Serialize data
    std::ofstream stream(File::GetExistingOrFirstPath(File::GetPath(filename)), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryOutputArchive archive(stream);
    archive(cereal::make_nvp("mesh", data));

    return true;
}

template<>
bool Resource<Mesh>::Load(const String& filename, Mesh& data)
{
    // Deserialize data
    std::ifstream stream(File::GetExistingPath(File::GetPath(filename)), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryInputArchive archive(stream);
    archive(cereal::make_nvp("mesh", data));

    // Build graphic components
    List<Mesh::Vertex> vertices;
    vertices.resize(data.m_vertices.size());

    // TODO: can we avoid doing this, can get very slow with a lot of assets
    for (SizeType i = 0; i < vertices.size(); i++)
    {
        auto& v = vertices[i];
        v.position = data.m_vertices[i];
        v.color = data.m_colors[i];
        v.normal = data.m_normals[i];
        v.tangent = data.m_tangents[i];
        v.uv = data.m_uvs[i];
        v.bones = data.m_bones[i];
        v.weights = data.m_weights[i];
    }

    BufferCreateInfo vertexCreateInfo{};
    vertexCreateInfo.name = Log::Format("{} Vertex Buffer", filename);
    vertexCreateInfo.size = vertices.size() * sizeof(Mesh::Vertex);
    vertexCreateInfo.usageFlags = BufferUsageFlags::VERTEX | BufferUsageFlags::COPY_SRC;
    vertexCreateInfo.data = static_cast<const void*>(vertices.data());
    data.m_vertexBuffer = Graphics::CreateBuffer(vertexCreateInfo);

    BufferCreateInfo indexCreateInfo{};
    indexCreateInfo.name = Log::Format("{} Index Buffer", filename);
    indexCreateInfo.size = data.m_indices.size() * sizeof(u32);
    indexCreateInfo.usageFlags = BufferUsageFlags::INDEX | BufferUsageFlags::COPY_SRC;
    indexCreateInfo.data = static_cast<const void*>(data.m_indices.data());
    data.m_indexBuffer = Graphics::CreateBuffer(indexCreateInfo);

    return true;
}

template<>
void Resource<Mesh>::Unload(Mesh& data)
{
    Graphics::DestroyBuffer(data.m_vertexBuffer);
    Graphics::DestroyBuffer(data.m_indexBuffer);
}