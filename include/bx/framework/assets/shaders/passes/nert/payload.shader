#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "[engine]/shaders/packing.shader"

struct Payload
{
	PackedRgb9e5 accumulated;
	PackedRgb9e5 throughput;
	uint rngState;
	PackedNormalizedXyz10 hitNormal;
};

#endif // PAYLOAD_H