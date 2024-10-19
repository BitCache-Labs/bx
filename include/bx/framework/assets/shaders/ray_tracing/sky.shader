#ifndef SKY_H
#define SKY_Y

#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/sampling.shader"

const float SUN_DISTANCE = 1000.0;

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

layout (BINDING(3, 1)) uniform texture2D skyTexture;

layout (BINDING(3, 2)) uniform sampler skySampler;

#endif // SKY_BINDINGS

float sunIntensity(float zenithAngleCos)
{
    float cutoffAngle = PI / 1.95;
    float intensity = skyConstants.sunIntensity *
        max(0.0, 1.0 - exp(-((cutoffAngle - acos(zenithAngleCos)) / 1.4)));
    return intensity;
}

float sunSolidAngle()
{
    return TWO_PI * (1.0 - cos(skyConstants.sunSize));
}

vec3 sampleSunDirection(vec2 uv)
{
	return normalize(perturbDirectionVector(uv, -skyConstants.sunDirection, skyConstants.sunSize));
}

vec3 shadeSky(vec3 direction)
{
	direction.y = -direction.y;
	vec2 uv = unitVectorToPanoramaCoords(direction);
	vec3 skyColor = texture(sampler2D(skyTexture, skySampler), uv).rgb;

	return skyColor;
}

#endif // SKY_H