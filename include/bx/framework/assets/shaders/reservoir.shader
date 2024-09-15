#ifndef RESERVOIR_H
#define RESERVOIR_H

#include "[engine]/shaders/random.shader"

struct ReservoirStreamData
{
	float selectedSelectionWeight;
	float sampleCountSum;
};

struct ReservoirCountAndContributionWeight
{
	float sampleCount;
	float contributionWeight;
};

struct Reservoir
{
	float sampleCount;
	float contributionWeight;
	float weightSum;
};

/// ReservoirStreamData
/// ------------------------------------------

ReservoirStreamData ReservoirStreamData_default()
{
	return ReservoirStreamData(0.0, 0.0);
}

ReservoirStreamData ReservoirStreamData_fromSingleSample(float selectionWeight)
{
	return ReservoirStreamData(selectionWeight, 1.0);
}

/// ReservoirCountAndContributionWeight
/// ------------------------------------------

ReservoirCountAndContributionWeight ReservoirCountAndContributionWeight_fromPacked(uint packed)
{
	vec2 countAndWeight = unpackHalf2x16(packed);
	return ReservoirCountAndContributionWeight(countAndWeight.x, countAndWeight.y);
}

uint ReservoirCountAndContributionWeight_toPacked(ReservoirCountAndContributionWeight self)
{
	return packHalf2x16(vec2(self.sampleCount, self.contributionWeight));
}

/// Reservoir
/// ------------------------------------------

Reservoir Reservoir_default()
{
	return Reservoir(0.0, 0.0, 0.0);
}

uint Reservoir_toPacked(Reservoir self)
{
	return packHalf2x16(vec2(self.sampleCount, self.contributionWeight));
}

Reservoir Reservoir_reconstructBiased(uint packedCountAndContributionWeight)
{
	ReservoirCountAndContributionWeight sampleReservoirData =
		ReservoirCountAndContributionWeight_fromPacked(packedCountAndContributionWeight);

	float biasedWeight = sampleReservoirData.sampleCount * sampleReservoirData.contributionWeight;
	Reservoir reservoir = Reservoir(sampleReservoirData.sampleCount, sampleReservoirData.contributionWeight, biasedWeight);
	return reservoir;
}

Reservoir Reservoir_reconstructBiasedClamped(uint packedCountAndContributionWeight, float sampleCountClamp)
{
	ReservoirCountAndContributionWeight sampleReservoirData =
		ReservoirCountAndContributionWeight_fromPacked(packedCountAndContributionWeight);
	sampleReservoirData.sampleCount = min(sampleReservoirData.sampleCount, sampleCountClamp);

	float biasedWeight = sampleReservoirData.sampleCount * sampleReservoirData.contributionWeight;
	Reservoir reservoir = Reservoir(sampleReservoirData.sampleCount, sampleReservoirData.contributionWeight, biasedWeight);
	return reservoir;
}

bool Reservoir_update(inout Reservoir self, float sampleWeight, inout uint rng)
{
	self.weightSum += sampleWeight;
	self.sampleCount += 1.0;

	return randomUniformFloat(rng) < (sampleWeight / self.weightSum);
}

void Reservoir_finishStream(inout Reservoir self, ReservoirStreamData streamData)
{
	self.sampleCount = streamData.sampleCountSum;
	self.contributionWeight = self.weightSum / max(1e-8, self.sampleCount * streamData.selectedSelectionWeight);
}

float Reservoir_contributionWeightSum(Reservoir self)
{
	return self.contributionWeight * self.sampleCount;
}

bool Reservoir_mergeReservoirWithStream(inout Reservoir self, inout ReservoirStreamData streamData, Reservoir reservoir, float visibility, float jacobian, float samplingWeight, inout uint rng)
{
	if (visibility * jacobian > 0.0)
	{
		float weight = samplingWeight * Reservoir_contributionWeightSum(reservoir) * visibility * jacobian;
		streamData.sampleCountSum += reservoir.sampleCount;

		if (Reservoir_update(self, weight, rng))
		{
			streamData.selectedSelectionWeight = samplingWeight * jacobian;
			return true;
		}
	}

	return false;
}

void Reservoir_clampContributionWeight(inout Reservoir self, float clampWeight)
{
	self.contributionWeight = min(self.contributionWeight, clampWeight);
}

void Reservoir_clampSampleCount(inout Reservoir self, float clampCount)
{
	self.sampleCount = min(self.sampleCount, clampCount);
}

#endif // RESERVOIR_H