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
	float weight;
	uint sampleCount;
	uint _PADDING1;
};

Reservoir makeReservoir()
{
	Reservoir reservoir;
	reservoir.outputSample = makeRestirSample();
	reservoir.weightSum = 0.0;
	reservoir.weight = 0.0;
	reservoir.sampleCount = 0;
	return reservoir;
}

bool isRestirSampleValid(RestirSample restirSample)
{
	return restirSample.weight != 0.0;
}

void updateReservoir(inout Reservoir reservoir, inout uint rngState, RestirSample restirSample, float weight)
{
	reservoir.weightSum += weight;
	reservoir.sampleCount += 1;

	if (randomUniformFloat(rngState) < (weight / reservoir.weightSum))
	{
		reservoir.outputSample = restirSample;
	}
}

Reservoir combineReservoirs(inout uint rngState, Reservoir a, Reservoir b)
{
	Reservoir result = makeReservoir();
	updateReservoir(result, rngState, a.outputSample, a.outputSample.unoccludedContributionWeight * a.weight * a.sampleCount);
	updateReservoir(result, rngState, b.outputSample, b.outputSample.unoccludedContributionWeight * b.weight * b.sampleCount);
	result.sampleCount = a.sampleCount + b.sampleCount;

	result.weight = (result.outputSample.unoccludedContributionWeight == 0.0) ? 0.0 : (1.0 / result.outputSample.unoccludedContributionWeight);
	//result.weight = 1.0 / result.outputSample.unoccludedContributionWeight;
	result.weight *= ((1.0 / result.sampleCount) * result.weightSum);
	
	return result;
}

#ifdef RESTIR_BINDINGS

layout (BINDING(4, 0), std430) buffer _RestirReservoirs
{
    Reservoir restirReservoirs[];
};

layout (BINDING(4, 1), std430) buffer _OutRestirReservoirs
{
    Reservoir outRestirReservoirs[];
};

layout (BINDING(4, 2), std430) buffer _RestirReservoirsHistory
{
    Reservoir restirReservoirsHistory[];
};

#endif // RESTIR_BINDINGS

#endif // RESTIR_H