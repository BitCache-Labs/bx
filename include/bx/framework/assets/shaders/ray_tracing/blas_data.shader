#ifndef BLAS_DATA_H
#define BLAS_DATA_H

#include "[engine]/shaders/ray_tracing/vertex.shader"
#include "[engine]/shaders/ray_tracing/triangle.shader"

struct BlasAccessor
{
	uint vertexOffset;
	uint vertexCount;
	uint triangleOffset;
	uint triangleCount;
};

struct BlasInstance
{
	mat4 transform;
	mat4 invTransTransform;
	uint blasIdx;
	uint materialIdx;
	uint _PADDING0;
	uint _PADDING1;
};

#ifdef BLAS_DATA_BINDINGS

layout (BINDING(1, 0), std140) uniform _BlasDataConstants
{
	uint emissiveTriangleCount;
    uint emissiveInstanceCount;
	uint _PADDING0;
	uint _PADDING1;
} blasDataConstants;

layout(BINDING(1, 1), std430) readonly buffer _BlasAccessors
{
    BlasAccessor blasAccessors[];
};

layout(BINDING(1, 2), std430) readonly buffer _BlasInstances
{
    BlasInstance blasInstances[];
};

layout(BINDING(1, 3), std430) readonly buffer _BlasTriangles
{
    Triangle blasTriangles[];
};

layout(BINDING(1, 4), std430) buffer _BlasVertices
{
    PackedVertex blasVertices[];
};

layout(BINDING(1, 5), std430) readonly buffer _BlasEmissiveInstanceIndices
{
	uint blasEmissiveInstanceIndices[];
};

#endif // BLAS_DATA_BINDINGS

#endif // BLAS_DATA_H