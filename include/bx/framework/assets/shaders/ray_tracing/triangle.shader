#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "[engine]/shaders/math.shader"

struct Triangle
{
	vec3 p0;
	uint i0;
	vec3 p1;
	uint i1;
	vec3 p2;
	uint i2;
};

Triangle Triangle_default()
{
	return Triangle(vec3(0.0), 0, vec3(0.0), 0, vec3(0.0), 0);
}

float calculateTriangleAreaFromEdges(vec3 edge1, vec3 edge2)
{
	return length(cross(edge1, edge2)) * 0.5;
}

float triangleLightSolidAngle(float cosOut, float lightArea, float lightDistance)
{
	return min(TWO_PI, (cosOut * lightArea) / (lightDistance * lightDistance));
}

Triangle transformedTriangle(Triangle triangle, mat4 transform)
{
	return Triangle(
		(transform * vec4(triangle.p0, 1.0)).xyz,
        triangle.i0,
        (transform * vec4(triangle.p1, 1.0)).xyz,
        triangle.i1,
        (transform * vec4(triangle.p2, 1.0)).xyz,
        triangle.i2
	);
}

#endif // TRIANGLE_H