#ifndef RAY_H
#define RAY_H

const float T_MISS = 1e30f;
const float RT_EPSILON = 0.0001;
const float RT_TINY_EPSILON = 0.000001;

struct Ray
{
	vec3 origin;
	uint _PADDING0;
	vec3 direction;
	uint _PADDING1;
};

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

#endif // RAY_H