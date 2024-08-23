#ifndef BSDF_H
#define BSDF_H

#include "[engine]/shaders/math.shader"
#include "[engine]/sampling.shader"

struct BsdfSample
{
	vec3 wInTangentSpace;
	float pdf;
	bool refract;
};

struct BsdfEval
{
	vec3 reflectance;
	vec3 fractionTransmitted;
};

bool applyBsdf(BsdfSample bsdfSample, BsdfEval bsdfEval,
	mat3 tangentToWorld, vec3 normal,
	inout vec3 throughput, out vec3 wInWorldSpace)
{
	// TODO: check bsdfSample valid

	wInWorldSpace = normalize(tangentToWorld * bsdfSample.wInTangentSpace);
	float cosIn = abs(dot(normal, wInWorldSpace));
	
	vec3 contribution = (1.0 / bsdfSample.pdf) * bsdfEval.reflectance * cosIn;
	throughput *= contribution;
	
	return true;
}

float diffuseBsdfPdf(float cosTheta)
{
	return max(0.0, cosTheta) * INV_PI;
}

vec3 diffuseBsdfEval(vec3 baseColor)
{
	return INV_PI * baseColor;
}

#endif // BSDF_H