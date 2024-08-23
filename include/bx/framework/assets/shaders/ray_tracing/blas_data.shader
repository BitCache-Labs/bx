#ifndef BLAS_DATA_H
#define BLAS_DATA_H

#include "[engine]/shaders/ray_tracing/vertex.shader"
#include "[engine]/shaders/ray_tracing/triangle.shader"

struct BlasAccessor
{
	uint vertexOffset;
	uint vertexCount;
	uint triangleOffset;
	uint _PADDING0;
};

struct BlasInstance
{
	mat4 invTransform;
	uint blasIdx;
	uint materialIdx;
	uint _PADDING0;
	uint _PADDING1;
};

#ifdef BLAS_DATA_BINDINGS

layout(BINDING(1, 0), std430) readonly buffer _BlasAccessors
{
    BlasAccessor blasAccessors[];
};

layout(BINDING(1, 1), std430) readonly buffer _BlasInstances
{
    BlasInstance blasInstances[];
};

layout(BINDING(1, 2), std430) readonly buffer _BlasTriangles
{
    Triangle blasTriangles[];
};

layout(BINDING(1, 3), std430) readonly buffer _BlasVertices
{
    Vertex blasVertices[];
};

#endif // BLAS_DATA_BINDINGS

#endif // BLAS_DATA_H