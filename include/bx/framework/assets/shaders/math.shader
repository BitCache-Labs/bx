#ifndef MATH_H
#define MATH_H

const float PI = 3.14159265358979323846264338327;
const float INV_PI = (1.0 / PI);
const float INV_2_PI = (1.0 / (2.0 * PI));
const float INV_4_PI = (1.0 / (4.0 * PI));
const float TWO_PI = 2.0 * PI;
const float HALF_PI = 0.5 * PI;

const float F32_MIN = -3.40282347E+38;
const float F32_MAX = 3.40282347E+38;
const float U32_MAX = 4294967295u;
const float U16_MAX = 65535u;

const float GOLDEN_RATIO = 1.6180339887498948482;

const vec3 RIGHT = vec3(1.0, 0.0, 0.0);
const vec3 UP = vec3(0.0, 1.0, 0.0);
const vec3 FORWARD = vec3(0.0, 0.0, 1.0);

float safeSqrt(float x)
{
	return sqrt(max(0.0, x));
}

bool isNan(float x)
{
	return x != x;
}

#endif // MATH_H