#ifndef TRIANGLE_H
#define TRIANGLE_H

struct Triangle
{
	vec3 p0;
	uint i0;
	vec3 p1;
	uint i1;
	vec3 p2;
	uint i2;
};

float areaOfTriangle(Triangle triangle)
{
	vec3 p01 = triangle.p1 - triangle.p0;
	vec3 p02 = triangle.p2 - triangle.p0;
	return length(cross(p01, p02)) * 0.5;
}

#endif // TRIANGLE_H