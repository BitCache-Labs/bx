#ifndef RESTIR_H
#define RESTIR_H

#include "[engine]/shaders/reservoir.shader"
#include "[engine]/shaders/packing.shader"

struct ReservoirData
{
    vec3 sampleDirection;
    float hitT;
};

uvec2 ReservoirData_toPacked(ReservoirData self)
{
    return uvec2(
        packNormalizedXyz10(self.sampleDirection, 0).data,
        floatBitsToUint(self.hitT)
    );
}

ReservoirData ReservoirData_fromPacked(uvec2 packed)
{
    return ReservoirData(
        unpackNormalizedXyz10(PackedNormalizedXyz10(packed.x), 0),
        uintBitsToFloat(packed.y)
    );
}

//struct RestirSample
//{
//	vec3 x0;
//	float weight;
//	vec3 x1;
//	float unoccludedContributionWeight;
//	vec3 x2;
//	uint _PADDING0;
//};
//
//RestirSample makeRestirSample()
//{
//	RestirSample restirSample;
//	restirSample.x0 = vec3(0.0);
//	restirSample.weight = 0.0;
//	restirSample.x1 = vec3(0.0);
//	restirSample.unoccludedContributionWeight = 0.0;
//	restirSample.x2 = vec3(0.0);
//	restirSample._PADDING0 = 0;
//	return restirSample;
//}
//
//struct Reservoir
//{
//	RestirSample outputSample;
//	float weightSum;
//	float weight;
//	uint sampleCount;
//	uint _PADDING0;
//};
//
//Reservoir makeReservoir()
//{
//	Reservoir reservoir;
//	reservoir.outputSample = makeRestirSample();
//	reservoir.weightSum = 0.0;
//	reservoir.weight = 0.0;
//	reservoir.sampleCount = 0;
//	reservoir._PADDING0 = 0;
//	return reservoir;
//}
//
//Reservoir makeInvalidReservoir()
//{
//	Reservoir reservoir = makeReservoir();
//	reservoir.sampleCount = U32_MAX;
//	return reservoir;
//}
//
//bool isReservoirValid(Reservoir reservoir)
//{
//	return reservoir.sampleCount != U32_MAX;
//}
//
//void updateReservoir(inout Reservoir reservoir, inout uint rngState, RestirSample restirSample, float weight)
//{
//	reservoir.weightSum += weight;
//	reservoir.sampleCount += 1;
//
//	if (randomUniformFloat(rngState) < (weight / reservoir.weightSum))
//	{
//		reservoir.outputSample = restirSample;
//		//reservoir.selectedSelectionWeight = reservoir.
//	}
//}
//
//Reservoir combineReservoirs(inout uint rngState, Reservoir a, Reservoir b)
//{
//	if (!isReservoirValid(a) && isReservoirValid(b)) return b;
//	if (!isReservoirValid(b) && isReservoirValid(a)) return a;
//	if (!isReservoirValid(a) && !isReservoirValid(b)) return makeReservoir(); // TODO: remove this
//
//	Reservoir result = makeReservoir();
//	updateReservoir(result, rngState, a.outputSample, a.outputSample.unoccludedContributionWeight * a.weight * a.sampleCount);
//	updateReservoir(result, rngState, b.outputSample, b.outputSample.unoccludedContributionWeight * b.weight * b.sampleCount);
//	result.sampleCount = a.sampleCount + b.sampleCount;
//
//	if (result.outputSample.unoccludedContributionWeight == 0.0 || result.sampleCount == 0)
//		result.weight = 0.0;
//	else
//		result.weight = (1.0 / result.outputSample.unoccludedContributionWeight) * ((1.0 / result.sampleCount) * result.weightSum);
//	//result.weight = (result.outputSample.unoccludedContributionWeight == 0.0) ? 0.0 : (1.0 / result.outputSample.unoccludedContributionWeight);
//	//result.weight *= ((result.sampleCount == 0) ? 0.0 : (1.0 / result.sampleCount)) * result.weightSum;
//
//	//if (isnan(result.weight))
//	//{
//	//	result.weight = 0.0;
//	//	result.sampleCount = 0;
//	//	result.weightSum = 0.0;
//	//}
//	//result.weight = fixNan(result.weight);
//	
//	return result;
//}

#ifdef RESTIR_BINDINGS

layout (BINDING(4, 0), std430) buffer _RestirReservoirs
{
    uint restirReservoirs[];
};

layout (BINDING(4, 1), std430) buffer _OutRestirReservoirs
{
    uint outRestirReservoirs[];
};

layout (BINDING(4, 2), std430) buffer _RestirReservoirsHistory
{
    uint restirReservoirsHistory[];
};

layout (BINDING(4, 3), std430) buffer _RestirReservoirData
{
    uvec2 restirReservoirData[];
};

layout (BINDING(4, 4), std430) buffer _OutRestirReservoirData
{
    uvec2 outRestirReservoirData[];
};

layout (BINDING(4, 5), std430) buffer _RestirReservoirDataHistory
{
    uvec2 restirReservoirDataHistory[];
};

#endif // RESTIR_BINDINGS

#endif // RESTIR_H