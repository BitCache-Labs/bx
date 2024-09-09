#include "bx/framework/systems/renderer/blas_data_pool.hpp"

#include "bx/framework/resources/mesh.hpp"

struct BlasDataConstants
{
    u32 emissiveTriangleCount;
    u32 emissiveInstanceCount;
    u32 _PADDING0;
    u32 _PADDING1;
};

BlasDataPool::BlasDataPool()
{
    BufferCreateInfo blasDataConstantsCreateInfo{};
    blasDataConstantsCreateInfo.name = "Blas Data Constants Buffer";
    blasDataConstantsCreateInfo.size = sizeof(BlasDataConstants);
    blasDataConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM | BufferUsageFlags::COPY_DST;
    blasDataConstantsBuffer = Graphics::CreateBuffer(blasDataConstantsCreateInfo);

    BufferCreateInfo blasAccessorsCreateInfo{};
    blasAccessorsCreateInfo.name = "Blas Accessors Buffer";
    blasAccessorsCreateInfo.size = MAX_BLAS_ACCESSORS * sizeof(BlasAccessor);
    blasAccessorsCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST;
    blasAccessorsBuffer = Graphics::CreateBuffer(blasAccessorsCreateInfo);

    BufferCreateInfo blasInstancesCreateInfo{};
    blasInstancesCreateInfo.name = "Blas Instances Buffer";
    blasInstancesCreateInfo.size = MAX_BLAS_INSTANCES * sizeof(BlasInstance);
    blasInstancesCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST;
    blasInstancesBuffer = Graphics::CreateBuffer(blasInstancesCreateInfo);

    BufferCreateInfo blasTrianglesCreateInfo{};
    blasTrianglesCreateInfo.name = "Blas Triangles Buffer";
    blasTrianglesCreateInfo.size = MAX_BLAS_TRIANGLES * sizeof(Mesh::Triangle);
    blasTrianglesCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST;
    blasTrianglesBuffer = Graphics::CreateBuffer(blasTrianglesCreateInfo);

    BufferCreateInfo blasVerticesCreateInfo{};
    blasVerticesCreateInfo.name = "Blas Vertices Buffer";
    blasVerticesCreateInfo.size = MAX_BLAS_VERTICES * sizeof(PackedVertex);
    blasVerticesCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST;
    blasVerticesBuffer = Graphics::CreateBuffer(blasVerticesCreateInfo);

    BufferCreateInfo blasEmissiveTriangleIndicesCreateInfo{};
    blasEmissiveTriangleIndicesCreateInfo.name = "Blas Emissive Instance Indices Buffer";
    blasEmissiveTriangleIndicesCreateInfo.size = MAX_BLAS_INSTANCES * sizeof(u32);
    blasEmissiveTriangleIndicesCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST;
    blasEmissiveInstanceIndicesBuffer = Graphics::CreateBuffer(blasEmissiveTriangleIndicesCreateInfo);
}

BlasDataPool::~BlasDataPool()
{
    Graphics::DestroyBuffer(blasDataConstantsBuffer);
    Graphics::DestroyBuffer(blasAccessorsBuffer);
    Graphics::DestroyBuffer(blasInstancesBuffer);
    Graphics::DestroyBuffer(blasTrianglesBuffer);
    Graphics::DestroyBuffer(blasVerticesBuffer);
    Graphics::DestroyBuffer(blasEmissiveInstanceIndicesBuffer);
}

BlasDataPool::BlasAccessor BlasDataPool::AllocateBlas(const Mesh& mesh)
{
    const List<Mesh::Vertex> vertices = mesh.BuildVertices();
    List<PackedVertex> packedVertices(vertices.size());
    for (u32 i = 0; i < vertices.size(); i++)
    {
        const auto& vertex = vertices[i];
        packedVertices[i].position = vertex.position;
        packedVertices[i].tangent = vertex.tangent;
        f16 uv[2] = { vertex.uv.x, vertex.uv.y };
        packedVertices[i].texCoord = Packing::Pack2xF16(uv);
        packedVertices[i].weights = vertex.weights;
        packedVertices[i].normal = PackedNormalizedXyz10(vertex.normal);
        packedVertices[i].color = PackedRgb9e5(vertex.color.Xyz());
        //packedVertices[i].bones = Packing::Pack4xU8(vertex.bones); TODO!
    }

    const List<Mesh::Triangle> triangles = mesh.BuildTriangles();

    BlasAccessor accessor{};
    accessor.vertexCount = vertices.size();
    accessor.vertexOffset = blasVerticesCount;
    accessor.triangleOffset = blasTriangleCount;
    accessor.triangleCount = triangles.size();
    
    Graphics::WriteBuffer(blasAccessorsBuffer, blasAccessorCount * sizeof(BlasAccessor), &accessor, sizeof(BlasAccessor));
    blasAccessorCount++;
    Graphics::WriteBuffer(blasVerticesBuffer, blasVerticesCount * sizeof(PackedVertex), packedVertices.data(), packedVertices.size() * sizeof(PackedVertex));
    blasVerticesCount += packedVertices.size();
    Graphics::WriteBuffer(blasTrianglesBuffer, blasTriangleCount * sizeof(Mesh::Triangle), triangles.data(), triangles.size() * sizeof(Mesh::Triangle));
    blasTriangleCount += triangles.size();

    return accessor;
}

u32 BlasDataPool::SubmitInstance(const Mesh& mesh, ResourceHandle resourceHandle, const Mat4& invTransform, u32 materialIdx, b8 isEmissive)
{
    u32 blasIdx;
    auto accessorIndexIter = blasAccessorIndices.find(resourceHandle);
    if (accessorIndexIter == blasAccessorIndices.end())
    {
        blasIdx = blasAccessors.size();

        BlasAccessor accessor = AllocateBlas(mesh);
        blasAccessors.push_back(accessor);
        blasAccessorIndices.insert(std::make_pair(resourceHandle, blasIdx));
    }
    else
    {
        blasIdx = accessorIndexIter->second;
    }

    BlasInstance blasInstance{};
    blasInstance.invTransform = invTransform;
    blasInstance.blasIdx = blasIdx;
    blasInstance.materialIdx = materialIdx;
    pendingInstances.push_back(blasInstance);

    if (isEmissive)
    {
        pendingEmissiveInstanceIndices.push_back(pendingInstances.size() - 1);
        pendingEmissiveTriangleCount += mesh.GetIndices().size() / 3;
    }

    return pendingInstances.size() - 1;
}

void BlasDataPool::Submit()
{
    if (!pendingInstances.empty())
    {
        Graphics::WriteBuffer(blasInstancesBuffer, 0, pendingInstances.data(), pendingInstances.size() * sizeof(BlasInstance));
        pendingInstances.clear();

        BlasDataConstants constants{};
        constants.emissiveInstanceCount = pendingEmissiveInstanceIndices.size();
        constants.emissiveTriangleCount = pendingEmissiveTriangleCount;
        Graphics::WriteBuffer(blasDataConstantsBuffer, 0, &constants);

        if (!pendingEmissiveInstanceIndices.empty())
        {
            Graphics::WriteBuffer(blasEmissiveInstanceIndicesBuffer, 0, pendingEmissiveInstanceIndices.data(), pendingEmissiveInstanceIndices.size() * sizeof(u32));
            pendingEmissiveInstanceIndices.clear();
            pendingEmissiveTriangleCount = 0;
        }
    }
}

BindGroupLayoutDescriptor BlasDataPool::GetBindGroupLayout()
{
    return BindGroupLayoutDescriptor(BIND_GROUP_SET, {
            BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),         // blasDataConstants
            BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // blasAccessors
            BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // blasInstances
            BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // blasTriangles
            BindGroupLayoutEntry(4, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // blasVertices
            BindGroupLayoutEntry(5, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true))      // blasEmissiveTriangleIndices
    });
}

BindGroupHandle BlasDataPool::CreateBindGroup(ComputePipelineHandle pipeline) const
{
    BindGroupCreateInfo createInfo{};
    createInfo.name = "Blas Data Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(pipeline, BIND_GROUP_SET);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(blasDataConstantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(blasAccessorsBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(blasInstancesBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(blasTrianglesBuffer)),
        BindGroupEntry(4, BindingResource::Buffer(blasVerticesBuffer)),
        BindGroupEntry(5, BindingResource::Buffer(blasEmissiveInstanceIndicesBuffer))
    };
    return Graphics::CreateBindGroup(createInfo);
}