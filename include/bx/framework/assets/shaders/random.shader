#ifndef RANDOM_H
#define RANDOM_H

uint wanghHash(uint x)
{
	uint state = (x ^ 61u) ^ (x >> 16);
	state *= 9u;
	state = state ^ (state >> 4);
    state *= 0x27d4eb2du;
    state = state ^ (state >> 15);
    return state;
}

uint pcgHash(uint x)
{
	uint state = x * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Combine hash, taken from Kajiya
uint hashCombine(uint x, uint y)
{
    const uint M = 1664525u, C = 1013904223u;
    uint seed = (x * M + y + C) * M;

    // Tempering (from Matsumoto)
    seed ^= (seed >> 11u);
    seed ^= (seed << 7u) & 0x9d2c5680u;
    seed ^= (seed << 15u) & 0xefc60000u;
    seed ^= (seed >> 18u);
    return seed;
}

uint xorShiftU32(uint x)
{
	uint s = x ^ (x << 13);
	s ^= s >> 17;
    s ^= s << 5;
    return s;
}

float randomUniformFloat(inout uint state)
{
	state = pcgHash(state);
	return float(state) * (1.0 / float(0xffffffffU));
}

vec2 randomUniformFloat2(inout uint state)
{
	return vec2(
		randomUniformFloat(state),
		randomUniformFloat(state)
	);
}

vec3 randomUniformFloat3(inout uint state)
{
	return vec3(
		randomUniformFloat(state),
		randomUniformFloat(state),
		randomUniformFloat(state)
	);
}

vec4 randomUniformFloat4(inout uint state)
{
	return vec4(
		randomUniformFloat(state),
		randomUniformFloat(state),
		randomUniformFloat(state),
		randomUniformFloat(state)
	);
}

#endif // RANDOM_H