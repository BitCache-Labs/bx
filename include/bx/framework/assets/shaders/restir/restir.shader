#ifndef RESTIR_H
#define RESTIR_H

#include "[engine]/shaders/random.shader"

struct RestirSample
{
	vec3 x0;
	float weight;
	vec3 x1;
	float unoccludedContributionWeight;
	vec3 x2;
	uint _PADDING0;
};

RestirSample makeRestirSample()
{
	RestirSample restirSample;
	restirSample.x0 = vec3(0.0);
	restirSample.weight = 0.0;
	restirSample.x1 = vec3(0.0);
	restirSample.unoccludedContributionWeight = 0.0;
	restirSample.x2 = vec3(0.0);
	restirSample._PADDING0 = 0;
	return restirSample;
}

struct Reservoir
{
	RestirSample outputSample;
	float weightSum;
};

Reservoir makeReservoir()
{
	Reservoir reservoir;
	reservoir.outputSample = makeRestirSample();
	reservoir.weightSum = 0.0;
	return reservoir;
}

bool isRestirSampleValid(RestirSample restirSample)
{
	return restirSample.weight != 0.0;
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