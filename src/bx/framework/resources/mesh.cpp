#include "bx/framework/resources/mesh.hpp"
#include "bx/framework/resources/mesh.serial.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/modules/graphics.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
#include <fstream>
#include <sstream>

List<Mesh::Vertex> Mesh::BuildVertices() const
{
    List<Vertex> vertices(m_vertices.size());
    for (SizeType i = 0; i < vertices.size(); i++)
    {
        auto& v = vertices[i];
        v.position = m_vertices[i];
        v.color = m_colors[i];
        v.normal = m_normals[i];
        v.tangent = m_tangents[i];
        v.uv = m_uvs[i];
        v.bones = m_bones[i];
        v.weights = m_weights[i];
    }
    return vertices;
}

List<Mesh::Triangle> Mesh::BuildTriangles() const
{
    List<Triangle> triangles(m_indices.size() / 3);
    for (SizeType i = 0; i < m_indices.size() / 3; i++)
    {
        auto& t = triangles[i];
        t.i0 = m_indices[i * 3 + 0];
        t.p0 = m_vertices[t.i0];
        t.i1 = m_indices[i * 3 + 1];
        t.p1 = m_vertices[t.i1];
        t.i2 = m_indices[i * 3 + 2];
        t.p2 = m_vertices[t.i2];
    }
    return triangles;
}

template<>
bool Resource<Mesh>::Save(const String& filename, const Mesh& data)
{
    // TODO: Use some sort of compression (research needs to be done)

    // Serialize data
    std::ofstream stream(File::GetPath(filename), std::ios::binary);
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
    std::ifstream stream(File::GetPath(filename), std::ios::binary);
    if (stream.fail())
        return false;

    cereal::PortableBinaryInputArchive archive(stream);
    archive(cereal::make_nvp("mesh", data));

    List<Mesh::Vertex> vertices = data.BuildVertices();

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

    BlasCreateInfo blasCreateInfo{};
    blasCreateInfo.name = Log::Format("{} Blas", filename);
    blasCreateInfo.vertexBuffer = data.m_vertexBuffer;
    blasCreateInfo.vertexFormat = VertexFormat::FLOAT_32X3;
    blasCreateInfo.vertexStride = sizeof(Mesh::Vertex);
    blasCreateInfo.indexBuffer = data.m_indexBuffer;
    blasCreateInfo.indexFormat = IndexFormat::UINT32;
    data.m_blas = Graphics::CreateBlas(blasCreateInfo);

    return true;
}

template<>
void Resource<Mesh>::Unload(Mesh& data)
{
    if (data.m_vertexBuffer)
    {
        Graphics::DestroyBuffer(data.m_vertexBuffer);
        Graphics::DestroyBuffer(data.m_indexBuffer);
        Graphics::DestroyBlas(data.m_blas);
    }
}