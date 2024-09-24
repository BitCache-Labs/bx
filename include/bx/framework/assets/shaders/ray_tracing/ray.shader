#ifndef RAY_H
#define RAY_H

#include "[engine]/shaders/packing.shader"

const float T_MISS = 1e30f;
const float RT_EPSILON = 0.0001;
const float RT_TINY_EPSILON = 0.000001;

struct PackedRay
{
	vec3 origin;
	PackedNormalizedXyz10 direction;
};

struct Ray
{
	vec3 origin;
	uint _PADDING0;
	vec3 direction;
	uint _PADDING1;
};

// TODO: pack
struct Intersection
{
	vec2 uv;
	uint primitiveIdx;
	uint blasInstanceIdx;
	float t;
	bool frontFace;
	uint _PADDING0;
	uint _PADDING1;
};

vec3 barycentricsFromUv(vec2 uv)
{
	return vec3(1.0 - uv.x - uv.y, uv.xy);
}

PackedRay packRay(Ray ray)
{
	PackedRay packedRay;
	packedRay.origin = ray.origin;
	packedRay.direction = packNormalizedXyz10(ray.direction, 0);
	return packedRay;
}

Ray unpackRay(PackedRay packedRay)
{
	Ray ray;
	ray.origin = packedRay.origin;
	ray.direction = unpackNormalizedXyz10(packedRay.direction, 0);
	return ray;
}

#endif // RAY_H