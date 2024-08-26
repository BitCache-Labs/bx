#ifndef SAMPLING_H
#define SAMPLING_H

#include "[engine]/shaders/math.shader"

vec3 getCosineHemisphereSample(vec2 uv)
{
    float phi = TWO_PI * uv.x;
    float sin_theta = sqrt(1.0 - uv.y);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    return vec3(
        sin_phi * sin_theta,
        cos_phi * sin_theta,
        safeSqrt(uv.y)
    ); 
}

vec3 getUniformHemisphereSample(vec2 uv)
{
	float phi = TWO_PI * uv.x;
    float r = sqrt(1.0 - uv.y * uv.y);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    return vec3(
        r * cos_phi,
        r * sin_phi,
        uv.y
    );
}

vec3 getUniformSphereSample(vec2 uv)
{
    float phi = TWO_PI * uv.x;
    float theta = acos(1.0 - 2.0 * uv.y);

    float sin_phi = sin(phi);
    float cos_phi = cos(phi);
    float sin_theta = sin(theta);
    float cos_theta = cos(theta);

    return vec3(
        sin_theta * cos_phi,
        sin_theta * sin_phi,
        cos_theta
    );
}

// from https://stackoverflow.com/a/2660181
vec3 perturbDirectionVector(vec2 uv, vec3 direction, float angle)
{
    float h = cos(angle);

    float phi = 2.0 * PI * uv.x;

    float z = h + (1.0 - h) * uv.y;
    float sinT = sqrt(1.0 - z * z);

    float x = cos(phi) * sinT;
    float y = sin(phi) * sinT;

    vec3 bitangent = getPerpendicularVector(direction);
    vec3 tangent = cross(bitangent, direction);

    return bitangent * x + tangent * y + direction * z;
}

#endif // SAMPLING_H