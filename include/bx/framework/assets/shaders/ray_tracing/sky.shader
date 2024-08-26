#ifndef SKY_H
#define SKY_Y

#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/sampling.shader"

struct SkyConstants
{
	vec3 sunDirection;
	float sunSize;
	vec3 sunColor;
	float sunIntensity;
};

#ifdef SKY_BINDINGS

layout (BINDING(3, 0), std140) uniform _SkyConstants
{
	SkyConstants skyConstants;
};

#endif // SKY_BINDINGS

float sunIntensity(float zenithAngleCos)
{
    float cutoffAngle = PI / 1.95;
    float intensity = skyConstants.sunIntensity *
        max(0.0, 1.0 - exp(-((cutoffAngle - acos(zenithAngleCos)) / 1.4)));
    return intensity;
}

vec3 sampleSunDirection(vec2 uv)
{
	return normalize(perturbDirectionVector(uv, -skyConstants.sunDirection, skyConstants.sunSize * 0.1));
}

#endif // SKY_H