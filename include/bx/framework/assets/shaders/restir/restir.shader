#ifndef RESTIR_H
#define RESTIR_H

#include "[engine]/shaders/random.shader"

struct RestirPath
{
	vec3 x0;
	uint _PADDING0;
	vec3 x1;
	uint _PADDING1;
	vec3 x2;
	uint _PADDING2;
};

struct RestirSample
{
	RestirPath path;
	float weight;
	float unoccludedContributionWeight;
	uint _PADDING0;
	uint _PADDING1;
};

struct Reservoir
{
	RestirSample outputSample;
	float weightSum;
};

void updateReservoir(inout Reservoir reservoir, inout uint rngState, RestirSample restirSample, float weight)
{
	reservoir.weightSum += weight;

	if (randomUniformFloat(rngState) < (weight / reservoir.weightSum))
	{
		reservoir.outputSample = restirSample;
	}
}

#ifdef RESTIR_BINDINGS

layout (BINDING(3, 0), std430) buffer _RestirSamples
{
    RestirSample restirSamples[];
};

layout (BINDING(3, 1), std430) buffer _RestirSamplesHistory
{
    RestirSample restirSamplesHistory[];
};

#endif // RESTIR_BINDINGS

#endif // RESTIR_H