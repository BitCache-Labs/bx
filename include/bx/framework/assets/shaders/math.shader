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

float sqr(float x)
{
    return x * x;
}

bool isNan(float x)
{
	return x != x;
}

uint divCeil(uint x, uint y)
{
	return (x + y - 1) / y;
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
//fn get_perpendicular_vector(u: vec3<f32>) -> vec3<f32> {
//    let a: vec3<f32> = abs(u);
//
//    // Be explicit about uint types in the ternary to work around
//    // https://github.com/microsoft/DirectXShaderCompiler/issues/4727
//    let xm: u32 = select(0u, 1u, ((a.x - a.y) < 0 && (a.x - a.z) < 0));
//    let ym: u32 = select(0u, 1 ^ xm, (a.y - a.z) < 0);
//    let zm: u32 = 1 ^ (xm | ym);
//
//    return cross(u, vec3<f32>(f32(xm), f32(ym), f32(zm)));
//}

#endif // MATH_H