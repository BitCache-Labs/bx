#ifndef PAYLOAD_H
#define PAYLOAD_H

struct Payload
{
	vec3 accumulated;
	uint _PADDING0;
	vec3 throughput;
	uint rngState;
};

#endif // PAYLOAD_H