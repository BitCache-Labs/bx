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
	float sampleCount;
	float contributionWeight;
};

Reservoir Reservoir_default()
{
	return Reservoir(0.0, 0.0, 0.0);
}

PackedReservoir Reservoir_toPacked(Reservoir self)
{
	return PackedReservoir(
		self.sampleCount,
		self.contributionWeight
	);
}

Reservoir Reservoir_fromPacked(PackedReservoir packed)
{
	float weightSum = packed.sampleCount * packed.contributionWeight;
	return Reservoir(packed.sampleCount, packed.contributionWeight, weightSum);
}

Reservoir Reservoir_fromPackedClamped(PackedReservoir packed, float sampleCountClamp)
{
	float sampleCount = min(packed.sampleCount, sampleCountClamp);
	float weightSum = sampleCount * packed.contributionWeight;
	return Reservoir(sampleCount, packed.contributionWeight, weightSum);
}

bool Reservoir_update(inout Reservoir self, float sampleWeight, float sampleCount, inout uint rng)
{
	self.weightSum += sampleWeight;
	self.sampleCount += sampleCount;

	return randomUniformFloat(rng) < (sampleWeight / self.weightSum);
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