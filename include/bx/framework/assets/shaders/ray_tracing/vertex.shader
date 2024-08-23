#ifndef VERTEX_H
#define VERTEX_H

struct Vertex
{
	vec3 position;
	uint _PADDING0;
	vec4 color;
	ivec4 bones;
	vec4 weights;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
};

#endif // VERTEX_H