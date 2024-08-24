#include "bx/framework/systems/renderer/material_pool.hpp"

#include "bx/framework/resources/material.hpp"

MaterialPool::MaterialPool()
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

//BlasDataPool::BlasAccessor BlasDataPool::AllocateBlas(const Mesh& mesh)
//{
//    const List<Mesh::Vertex> vertices = mesh.BuildVertices();
//    const List<Mesh::Triangle> triangles = mesh.BuildTriangles();
//
//    BlasAccessor accessor{};
//    accessor.vertexCount = vertices.size();
//    accessor.vertexOffset = blasVerticesCount;
//    accessor.triangleOffset = blasTriangleCount;
//
//    Graphics::WriteBuffer(blasAccessorsBuffer, blasAccessorCount * sizeof(BlasAccessor), &accessor, sizeof(BlasAccessor));
//    blasAccessorCount++;
//    Graphics::WriteBuffer(blasVerticesBuffer, blasVerticesCount * sizeof(Mesh::Vertex), vertices.data(), vertices.size() * sizeof(Mesh::Vertex));
//    blasVerticesCount += vertices.size();
//    Graphics::WriteBuffer(blasTrianglesBuffer, blasTriangleCount * sizeof(Mesh::Triangle), triangles.data(), triangles.size() * sizeof(Mesh::Triangle));
//    blasTriangleCount += triangles.size();
//
//    return accessor;
//}

MaterialPool::MaterialDescriptor MaterialPool::AllocateMaterial(const Material& material)
{
    MaterialDescriptor materialDescriptor{};
    materialDescriptor.baseColorFactor = material.GetBaseColorFactor().Xyz();
    
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
            BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),     // materialDescriptors
        });
}

BindGroupHandle MaterialPool::CreateBindGroup(ComputePipelineHandle pipeline) const
{
    BindGroupCreateInfo createInfo{};
    createInfo.name = "Material Bind Group";
    createInfo.layout = Graphics::GetBindGroupLayout(pipeline, BIND_GROUP_SET);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(materialDescriptorsBuffer)),
    };
    return Graphics::CreateBindGroup(createInfo);
}