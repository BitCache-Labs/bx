#ifndef LAYERED_LOBE_H
#define LAYERED_LOBE_H

#include "[engine]/shaders/ray_tracing/material/bsdf.shader"

struct DiffuseLobe
{
	vec3 albedo;
};

BsdfSample sampleDiffuseBsdf(DiffuseLobe lobe, vec2 uv)
{
	BsdfSample bsdfSample;
	bsdfSample.wInTangentSpace = getCosineHemisphereSample(uv);
	bsdfSample.pdf = diffuseBsdfPdf(bsdfSample.wInTangentSpace.z);
	bsdfSample.refract = false;
	return bsdfSample;
}

BsdfEval evalDiffuseBsdf(DiffuseLobe lobe)
{
	BsdfEval bsdfEval;
	bsdfEval.reflectance = diffuseBsdfEval(lobe.albedo);
	bsdfEval.fractionTransmitted = vec3(0.0);
	return bsdfEval;
}

#endif // LAYERED_LOBE_H