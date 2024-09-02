#ifndef RESTIR_H
#define RESTIR_H

#include "[engine]/shaders/random.shader"

struct RestirSample
{
	vec4 x0;
	vec4 x1;
	vec4 x2;
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

RestirSample invalidRestirSample()
{
	RestirSample restirSample;
	restirSample.weight = -1.0;
	return restirSample;
}

bool isRestirSampleValid(RestirSample restirSample)
{
	return restirSample.weight != -1.0;
}

void updateReservoir(inout Reservoir reservoir, inout uint rngState, RestirSample restirSample, float weight)
{
	reservoir.weightSum += weight;

	if (randomUniformFloat(rngState) < (weight / reservoir.weightSum))
	{
		reservoir.outputSample = restirSample;
	}
}

#ifdef RESTIR_BINDINGS

layout (BINDING(4, 0), std430) buffer _RestirSamples
{
    RestirSample restirSamples[];
};

layout (BINDING(4, 1), std430) buffer _OutRestirSamples
{
    RestirSample outRestirSamples[];
};

layout (BINDING(4, 2), std430) buffer _RestirSamplesHistory
{
    RestirSample restirSamplesHistory[];
};

#endif // RESTIR_BINDINGS

#endif // RESTIR_H