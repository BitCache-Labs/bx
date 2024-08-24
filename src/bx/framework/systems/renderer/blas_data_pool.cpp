#include "bx/framework/systems/renderer/blas_data_pool.hpp"

#include "bx/framework/resources/mesh.hpp"

BlasDataPool::BlasDataPool()
{
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
    blasVerticesCreateInfo.size = MAX_BLAS_VERTICES * sizeof(Mesh::Vertex);
    blasVerticesCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST;
    blasVerticesBuffer = Graphics::CreateBuffer(blasVerticesCreateInfo);
}

BlasDataPool::~BlasDataPool()
{
    Graphics::DestroyBuffer(blasAccessorsBuffer);
    Graphics::DestroyBuffer(blasInstancesBuffer);
    Graphics::DestroyBuffer(blasTrianglesBuffer);
    Graphics::DestroyBuffer(blasVerticesBuffer);
}

BlasDataPool::BlasAccessor BlasDataPool::AllocateBlas(const Mesh& mesh)
{
    const List<Mesh::Vertex> vertices = mesh.BuildVertices();
    const List<Mesh::Triangle> triangles = mesh.BuildTriangles();

    BlasAccessor accessor{};
    accessor.vertexCount = vertices.size();
    accessor.vertexOffset = blasVerticesCount;
    accessor.triangleOffset = blasTriangleCount;
    
    Graphics::WriteBuffer(blasAccessorsBuffer, blasAccessorCount * sizeof(BlasAccessor), &accessor, sizeof(BlasAccessor));
    blasAccessorCount++;
    Graphics::WriteBuffer(blasVerticesBuffer, blasVerticesCount * sizeof(Mesh::Vertex), vertices.data(), vertices.size() * sizeof(Mesh::Vertex));
    blasVerticesCount += vertices.size();
    Graphics::WriteBuffer(blasTrianglesBuffer, blasTriangleCount * sizeof(Mesh::Triangle), triangles.data(), triangles.size() * sizeof(Mesh::Triangle));
    blasTriangleCount += triangles.size();

    return accessor;
}

void BlasDataPool::SubmitInstance(const Mesh& mesh, ResourceHandle resourceHandle, const Mat4& invTransform, u32 materialIdx)
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
}

void BlasDataPool::Submit()
{
    if (!pendingInstances.empty())
    {
        Graphics::WriteBuffer(blasInstancesBuffer, 0, pendingInstances.data(), pendingInstances.size() * sizeof(BlasInstance));
        pendingInstances.clear();
    }
}

BindGroupLayoutDescriptor BlasDataPool::GetBindGroupLayout()
{
    return BindGroupLayoutDescriptor(BIND_GROUP_SET, {
            BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // blasAccessors
            BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // blasInstances
            BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // blasTriangles
            BindGroupLayoutEntry(3, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true))      // blasVertices
    });
}

BindGroupHandle BlasDataPool::CreateBindGroup(ComputePipelineHandle pipeline) const
{
    BindGroupCreateInfo createInfo{};
    createInfo.name = "Blas Data Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(pipeline, BIND_GROUP_SET);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(blasAccessorsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(blasInstancesBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(blasTrianglesBuffer)),
        BindGroupEntry(3, BindingResource::Buffer(blasVerticesBuffer)),
    };
    return Graphics::CreateBindGroup(createInfo);
}