#ifndef RESTIR_H
#define RESTIR_H

#include "[engine]/shaders/random.shader"

struct RestirSample
{
	uint data;
};

struct Reservoir
{
	RestirSample outputSample;

	float weightSum;
	uint sampleCount;
};

void updateReservoir(inout Reservoir reservoir, inout uint rngState, RestirSample restirSample, float weight)
{
	reservoir.weightSum += weight;
	reservoir.sampleCount += 1;

	if (randomUniformFloat(rngState) < (weight / reservoir.weightSum))
	{
		reservoir.outputSample = restirSample;
	}
}

#endif // RESTIR_H