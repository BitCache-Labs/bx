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
    blasTrianglesCreateInfo.size = MAX_BLAS_TRIANGLES * sizeof(Triangle);
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