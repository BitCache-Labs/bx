#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/math.hpp"
#include "bx/engine/core/resource.hpp"

#include "bx/engine/modules/graphics.hpp"
#include "bx/framework/components/animator.hpp"


class Mesh;

class BlasDataPool : NoCopy
{
public:
	struct BlasAccessor
	{
		u32 vertexOffset;
		u32 vertexCount;
		u32 triangleOffset;
		u32 triangleCount;
	};

	struct BlasInstance
	{
		Mat4 transform;
		Mat4 invTransTransform;
		u32 blasIdx;
		u32 materialIdx;
		u32 _PADDING0;
		u32 _PADDING1;
	};

	struct PackedVertex
	{
		Vec3 position;
		u32 _PADDING0;
		Vec3 tangent; // TODO: encode
		u32 texCoord;
		Vec4 weights; // TODO: encode
		PackedNormalizedXyz10 normal;
		PackedRgb9e5 color;
		u32 bones;
		u32 _PADDING1;
	};

public:
	BlasDataPool();
	~BlasDataPool();

	u32 SubmitInstance(const Resource<Mesh>& meshResource, const Mat4& transform, const Mat4& invTransTransform, u32 materialIdx, b8 isEmissive);
	u32 SubmitAnimatedInstance(const Resource<Mesh>& meshResource, const Animator& animator, EntityId entityId, const Mat4& transform, const Mat4& invTransTransform, u32 materialIdx, b8 isEmissive);
	void Submit();

	BindGroupHandle CreateBindGroup(ComputePipelineHandle pipeline) const;

	static BindGroupLayoutDescriptor GetBindGroupLayout();
	constexpr static u32 BIND_GROUP_SET = 1;

	constexpr static u32 MAX_BLAS_ACCESSORS = 1024 * 32;
	constexpr static u32 MAX_BLAS_INSTANCES = 1024 * 128;
	constexpr static u32 MAX_BLAS_TRIANGLES = 1024 * 1024 * 16;
	constexpr static u32 MAX_BLAS_VERTICES = 1024 * 1024;

private:
	BufferHandle blasDataConstantsBuffer = BufferHandle::null;
	BufferHandle blasAccessorsBuffer = BufferHandle::null;
	BufferHandle blasInstancesBuffer = BufferHandle::null;
	BufferHandle blasTrianglesBuffer = BufferHandle::null;
	BufferHandle blasVerticesBuffer = BufferHandle::null;
	BufferHandle blasEmissiveInstanceIndicesBuffer = BufferHandle::null;

	u32 blasAccessorCount = 0;
	u32 blasTriangleCount = 0;
	u32 blasVerticesCount = 0;

	std::unordered_map<ResourceHandle, u32> blasAccessorIndices;
	List<BlasAccessor> blasAccessors;

	List<BlasInstance> pendingInstances{};
	List<u32> pendingEmissiveInstanceIndices{};
	u32 pendingEmissiveTriangleCount = 0;

	BindGroupHandle updateAnimatedBindGroup;

	BlasAccessor AllocateBlas(const Mesh& mesh);
};