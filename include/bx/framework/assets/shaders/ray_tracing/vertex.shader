#ifndef VERTEX_H
#define VERTEX_H

#include "[engine]/shaders/packing.shader"

struct Vertex
{
	vec3 position;
	uint _PADDING0;
	vec4 color;
	ivec4 bones;
	vec4 weights;
	vec3 normal;
	uint _PADDING1;
	vec3 tangent;
	uint _PADDING2;
	vec2 texCoord;
	uint _PADDING3;
	uint _PADDING4;
};

struct PackedVertex
{
	vec3 position;
	uint _PADDING0;
	vec3 tangent; // TODO: encode
	uint texCoord;
	vec4 weights; // TODO: encode
	PackedNormalizedXyz10 normal;
	PackedRgb9e5 color;
	uint bones;
	uint _PADDING1;
};

#endif // VERTEX_H