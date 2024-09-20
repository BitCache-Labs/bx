#ifndef RESERVOIR_H
#define RESERVOIR_H

#include "[engine]/shaders/random.shader"

struct Reservoir
{
	float sampleCount;
	float contributionWeight;
	float weightSum;
};

struct PackedReservoir
{
	uint sampleCountAndContributionWeight;
	float weightSum;
};

Reservoir Reservoir_default()
{
	return Reservoir(0.0, 0.0, 0.0);
}

PackedReservoir Reservoir_toPacked(Reservoir self)
{
	return PackedReservoir(
		packHalf2x16(vec2(self.sampleCount, self.contributionWeight)),
		self.weightSum
	);
}

Reservoir Reservoir_fromPacked(PackedReservoir packed)
{
	vec2 sampleCountAndContributionWeight = unpackHalf2x16(packed.sampleCountAndContributionWeight);
	return Reservoir(sampleCountAndContributionWeight.x, sampleCountAndContributionWeight.y, packed.weightSum);
}

Reservoir Reservoir_fromPackedClamped(PackedReservoir packed, float sampleCountClamp)
{
	vec2 sampleCountAndContributionWeight = unpackHalf2x16(packed.sampleCountAndContributionWeight);
	return Reservoir(min(sampleCountAndContributionWeight.x, sampleCountClamp), sampleCountAndContributionWeight.y, packed.weightSum);
}

bool Reservoir_update(inout Reservoir self, float sampleWeight, inout uint rng)
{
	self.weightSum += sampleWeight;
	self.sampleCount += 1.0;

	return randomUniformFloat(rng) < (sampleWeight / self.weightSum);
}

Reservoir Reservoir_combineReservoirs(Reservoir reservoir0, float weight0, Reservoir reservoir1, float weight1,
	inout uint rng, out bool firstReservoirWasPicked)
{
	Reservoir result = Reservoir_default();

	Reservoir_update(result, weight0 * reservoir0.contributionWeight * reservoir0.sampleCount, rng);
	firstReservoirWasPicked = !Reservoir_update(result, weight1 * reservoir1.contributionWeight * reservoir1.sampleCount, rng);
	result.sampleCount = reservoir0.sampleCount + reservoir1.sampleCount;

	float pickedWeight = firstReservoirWasPicked ? weight0 : weight1;

	// TODO: avoid branch with max?
	if (pickedWeight == 0.0 || result.sampleCount == 0.0)
	{
		result.contributionWeight = 0.0;
	}
	else
	{
		result.contributionWeight = (1.0 / pickedWeight) * ((1.0 / result.sampleCount) * result.weightSum);
	}

	return result;
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