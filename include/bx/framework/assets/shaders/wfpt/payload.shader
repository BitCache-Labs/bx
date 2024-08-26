#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "[engine]/shaders/packing.shader"

struct Payload
{
	PackedRgb9e5 accumulated;
	PackedRgb9e5 throughput;
	uint rngState;
	uint _PADDING0;
};

#endif // PAYLOAD_H