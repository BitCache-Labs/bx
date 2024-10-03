#include "bx/framework/systems/renderer/blas_data_pool.hpp"

#include "bx/framework/resources/mesh.hpp"
#include "bx/engine/core/hash.hpp"
#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

struct BlasDataConstants
{
    u32 emissiveTriangleCount;
    u32 emissiveInstanceCount;
    u32 _PADDING0;
    u32 _PADDING1;
};

struct UpdateAnimatedConstants
{
    u32 vertexCount;
    u32 blasIdx;
    u32 originalBlasIdx;
    u32 _PADDING1;
};

struct UpdateAnimatedPipeline : public LazyInit<UpdateAnimatedPipeline, ComputePipelineHandle>
{
    UpdateAnimatedPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Blas Data Update Animated Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/blas_data_pool/update_animated.comp.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()), // constants
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()), // bones
            }),
            BlasDataPool::GetBindGroupLayout()
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Blas Data Update Animated Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<UpdateAnimatedPipeline> LazyInit<UpdateAnimatedPipeline, ComputePipelineHandle>::cache = nullptr;

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
    blasVerticesCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST | BufferUsageFlags::VERTEX;
    blasVerticesBuffer = Graphics::CreateBuffer(blasVerticesCreateInfo);

    BufferCreateInfo blasEmissiveTriangleIndicesCreateInfo{};
    blasEmissiveTriangleIndicesCreateInfo.name = "Blas Emissive Instance Indices Buffer";
    blasEmissiveTriangleIndicesCreateInfo.size = MAX_BLAS_INSTANCES * sizeof(u32);
    blasEmissiveTriangleIndicesCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::COPY_DST;
    blasEmissiveInstanceIndicesBuffer = Graphics::CreateBuffer(blasEmissiveTriangleIndicesCreateInfo);

    updateAnimatedBindGroup = CreateBindGroup(UpdateAnimatedPipeline::Get());
}

BlasDataPool::~BlasDataPool()
{
    Graphics::DestroyBindGroup(updateAnimatedBindGroup);

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
        u8 bones[4] = { Math::Max(vertex.bones.x, 0), Math::Max(vertex.bones.y, 0), Math::Max(vertex.bones.z, 0), Math::Max(vertex.bones.w, 0) }; // TODO: care about explicit -1 bones?
        packedVertices[i].bones = Packing::Pack4xU8(bones);
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

u32 BlasDataPool::SubmitInstance(const Resource<Mesh>& meshResource, const Mat4& transform, const Mat4& invTransTransform, u32 materialIdx, b8 isEmissive)
{
    const Mesh& mesh = meshResource.GetData();

    u32 blasIdx;
    auto accessorIndexIter = blasAccessorIndices.find(meshResource.GetHandle());
    if (accessorIndexIter == blasAccessorIndices.end())
    {
        blasIdx = blasAccessors.size();

        BlasAccessor accessor = AllocateBlas(mesh);
        blasAccessors.push_back(accessor);
        blasAccessorIndices.insert(std::make_pair(meshResource.GetHandle(), blasIdx));
    }
    else
    {
        blasIdx = accessorIndexIter->second;
    }

    BlasInstance blasInstance{};
    blasInstance.transform = transform;
    blasInstance.invTransTransform = invTransTransform;
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

u32 BlasDataPool::SubmitAnimatedInstance(const Resource<Mesh>& meshResource, const Animator& animator, EntityId entityId, const Mat4& transform, const Mat4& invTransTransform, u32 materialIdx, b8 isEmissive)
{
    Mesh& mesh = meshResource.GetData();

    SizeType combinedResourceHandle = meshResource.GetHandle();
    hashCombine(combinedResourceHandle, entityId);

    u32 blasIdx;
    auto accessorIndexIter = blasAccessorIndices.find(combinedResourceHandle);
    if (accessorIndexIter == blasAccessorIndices.end())
    {
        blasIdx = blasAccessors.size();

        BlasAccessor accessor = AllocateBlas(mesh);
        blasAccessors.push_back(accessor);
        blasAccessorIndices.insert(std::make_pair(combinedResourceHandle, blasIdx));
    }
    else
    {
        blasIdx = accessorIndexIter->second;
    }

    u32 originalBlasIdx;
    accessorIndexIter = blasAccessorIndices.find(meshResource.GetHandle());
    if (accessorIndexIter == blasAccessorIndices.end())
    {
        originalBlasIdx = blasAccessors.size();

        BlasAccessor accessor = AllocateBlas(mesh);
        blasAccessors.push_back(accessor);
        blasAccessorIndices.insert(std::make_pair(meshResource.GetHandle(), originalBlasIdx));
    }
    else
    {
        originalBlasIdx = accessorIndexIter->second;
    }

    {
        UpdateAnimatedConstants updateAnimatedConstants{};
        updateAnimatedConstants.vertexCount = blasAccessors[blasIdx].vertexCount;
        updateAnimatedConstants.blasIdx = blasIdx;
        updateAnimatedConstants.originalBlasIdx = originalBlasIdx;

        BufferCreateInfo updateAnimatedConstantsCreateInfo{};
        updateAnimatedConstantsCreateInfo.name = "Blas Data Update Animated Constants Buffer";
        updateAnimatedConstantsCreateInfo.data = &updateAnimatedConstants;
        updateAnimatedConstantsCreateInfo.size = sizeof(UpdateAnimatedConstants);
        updateAnimatedConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
        BufferHandle updateAnimatedConstantsBuffer = Graphics::CreateBuffer(updateAnimatedConstantsCreateInfo);

        BindGroupCreateInfo bindGroupCreateInfo{};
        bindGroupCreateInfo.name = "Blas Data Update Animated Bind Group";
        bindGroupCreateInfo.layout = Graphics::GetBindGroupLayout(UpdateAnimatedPipeline::Get(), 0);
        bindGroupCreateInfo.entries = {
            BindGroupEntry(0, BindingResource::Buffer(updateAnimatedConstantsBuffer)),
            BindGroupEntry(1, BindingResource::Buffer(animator.GetBoneBuffer())),
        };
        BindGroupHandle bindGroup = Graphics::CreateBindGroup(bindGroupCreateInfo);

        ComputePassDescriptor computePassDescriptor{};
        computePassDescriptor.name = "Blas Data Update Animated";
        ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
        {
            Graphics::SetComputePipeline(UpdateAnimatedPipeline::Get());
            Graphics::SetBindGroup(0, bindGroup);
            Graphics::SetBindGroup(BlasDataPool::BIND_GROUP_SET, updateAnimatedBindGroup);
            Graphics::DispatchWorkgroups(Math::DivCeil(updateAnimatedConstants.vertexCount, 128), 1, 1);
        }
        Graphics::EndComputePass(computePass);

        Graphics::DestroyBindGroup(bindGroup);
        Graphics::DestroyBuffer(updateAnimatedConstantsBuffer);
    }

    {
        BlasCreateInfo blasCreateInfo{};
        //blasCreateInfo.name = Log::Format("{} Blas", filename);
        blasCreateInfo.vertexBuffer = BufferSlice(blasVerticesBuffer, blasAccessors[blasIdx].vertexOffset * sizeof(PackedVertex), Optional<u64>::Some(blasAccessors[blasIdx].vertexCount * sizeof(PackedVertex)));
        blasCreateInfo.vertexFormat = VertexFormat::FLOAT_32X3;
        blasCreateInfo.vertexStride = sizeof(PackedVertex);
        blasCreateInfo.indexBuffer = mesh.GetIndexBuffer();
        blasCreateInfo.indexFormat = IndexFormat::UINT32;
        Graphics::DestroyBlas(mesh.m_blas);
        mesh.m_blas = Graphics::CreateBlas(blasCreateInfo);
    }

    BlasInstance blasInstance{};
    blasInstance.transform = transform;
    blasInstance.invTransTransform = invTransTransform;
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