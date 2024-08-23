#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/math.hpp"

#include "bx/engine/modules/graphics.hpp"

class BlasDataPool : NoCopy
{
public:
	struct BlasAccessor
	{
		u32 vertexOffset;
		u32 vertexCount;
		u32 indexOffset;
		u32 indexCount;
		u32 triangleOffset;
		u32 _PADDING0;
	};

	struct BlasInstance
	{
		Mat4 invTransform;
		u32 blasIdx;
		u32 materialIdx;
	};

	struct Triangle
	{
		Vec3 p0;
		u32 i0;
		Vec3 p1;
		u32 i1;
		Vec3 p2;
		u32 i2;
	};

public:
	BlasDataPool();
	~BlasDataPool();

	BindGroupHandle CreateBindGroup(ComputePipelineHandle pipeline) const;

	static BindGroupLayoutDescriptor GetBindGroupLayout();
	constexpr static u32 BIND_GROUP_SET = 1;

	constexpr static u32 MAX_BLAS_ACCESSORS = 1024 * 32;
	constexpr static u32 MAX_BLAS_INSTANCES = 1024 * 128;
	constexpr static u32 MAX_BLAS_TRIANGLES = 1024 * 1024 * 16;
	constexpr static u32 MAX_BLAS_VERTICES = MAX_BLAS_TRIANGLES * 3;

private:
	BufferHandle blasAccessorsBuffer = BufferHandle::null;
	BufferHandle blasInstancesBuffer = BufferHandle::null;
	BufferHandle blasTrianglesBuffer = BufferHandle::null;
	BufferHandle blasVerticesBuffer = BufferHandle::null;
};