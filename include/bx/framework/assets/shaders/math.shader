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
const uint U32_MAX = 4294967295u;
const uint U16_MAX = 65535u;

const float GOLDEN_RATIO = 1.6180339887498948482;
const float GOLDEN_ANGLE = 2.39996322972865332;

const vec3 RIGHT = vec3(1.0, 0.0, 0.0);
const vec3 UP = vec3(0.0, 1.0, 0.0);
const vec3 FORWARD = vec3(0.0, 0.0, 1.0);

float safeSqrt(float x)
{
	return sqrt(max(0.0, x));
}

float distanceSqr(vec3 a, vec3 b)
{
    vec3 delta = b - a;
    return dot(delta, delta);
}

float sqr(float x)
{
    return x * x;
}

vec3 sqr(vec3 x)
{
    return x * x;
}

bool isNan(float x)
{
    return isnan(x);
}

bool isNan(vec3 x)
{
    return isNan(x.x) || isNan(x.y) || isNan(x.z);
}

vec3 fixNan(vec3 x)
{
    return isNan(x) ? vec3(0.0) : x;
}

float fixNan(float x)
{
    return isNan(x) ? 0.0 : x;
}

uint divCeil(uint x, uint y)
{
	return (x + y - 1) / y;
}

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec2 saturate(vec2 x)
{
    return clamp(x, 0.0, 1.0);
}

vec3 saturate(vec3 x)
{
    return clamp(x, 0.0, 1.0);
}

vec4 saturate(vec4 x)
{
    return clamp(x, 0.0, 1.0);
}

float stdDev(float a, float b)
{
    return safeSqrt(abs(b - sqr(a)));
}

vec3 _reflect(vec3 v, vec3 n)
{
    return v - 2.0 * dot(v, n) * n;
}

vec3 _refract(vec3 uv, vec3 normal, float ior)
{
    float cos_theta = min(dot(-uv, normal), 1.0);
    vec3 r_perp = ior * (uv + cos_theta * normal);
    vec3 r_parl = -sqrt(abs(1.0 - dot(r_perp, r_perp))) * normal;
    return r_perp + r_parl;
}

vec2 pixelToUv(ivec2 pixel, uvec2 resolution)
{
    return (vec2(pixel) + 0.5) / vec2(resolution);
}

ivec2 rescaleResolution(ivec2 pixel, uvec2 inResolution, uvec2 outResolution)
{
     return ivec2(round(vec2(pixel) / vec2(inResolution) * vec2(outResolution)));
}

// http://jcgt.org/published/0006/01/01/
mat3 buildOrthonormalBasis(vec3 n)
{
    vec3 t1;
    vec3 t2;
    if (n.z < 0.0)
    {
        float a = 1.0 / (1.0 - n.z);
        float b = n.x * n.y * a;
        t1 = vec3(1.0 - n.x * n.x * a, -b, n.x);
        t2 = vec3(b, n.y * n.y * a - 1.0, -n.y);
    }
    else
    {
        float a = 1.0 / (1.0 + n.z);
        float b = -n.x * n.y * a;
        t1 = vec3(1.0 - n.x * n.x * a, b, -n.x);
        t2 = vec3(b, 1.0 - n.y * n.y * a, -n.y);
    }

    return mat3(t1, t2, n);
}

// (from "Efficient Construction of Perpendicular Vectors Without Branching", 2009)
vec3 getPerpendicularVector(vec3 u)
{
    vec3 a = abs(u);
    
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1u : 0u;//select(0u, 1u, );
    uint ym = (a.y - a.z) < 0 ? 1 ^ xm : 0u;//select(0u, 1 ^ xm, );
    uint zm = 1 ^ (xm | ym);
    
    return cross(u, vec3(float(xm), float(ym), float(zm)));
}

#endif // MATH_H