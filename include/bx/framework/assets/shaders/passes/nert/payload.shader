#ifndef NERT_PAYLOAD_H
#define NERT_PAYLOAD_H

#include "[engine]/shaders/packing.shader"

struct Payload
{
	PackedRgb9e5 accumulated;
	PackedRgb9e5 throughput;
	uint rngState;
	PackedNormalizedXyz10 hitNormal;
};

#endif // NERT_PAYLOAD_H