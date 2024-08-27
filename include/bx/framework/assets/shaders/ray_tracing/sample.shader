#ifndef SAMPLE_H
#define SAMPLE_H

struct Sample
{
	vec3 directionToLight;
	float distanceToLight;
	float pdf;
};

#endif // SAMPLE_H