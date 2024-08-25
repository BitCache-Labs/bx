#ifndef LAYERED_LOBE_H
#define LAYERED_LOBE_H

#include "[engine]/shaders/ray_tracing/material/bsdf.shader"
#include "[engine]/shaders/color_helpers.shader"

struct DiffuseLobe
{
	vec3 albedo;
};

struct SpecularLobe
{
	vec3 albedo;
	float roughnessFactor;
	float metallicFactor;
	float ior;
	float thickness;
};

struct TransmissionLobe
{
	float ior;
};

struct LayeredLobe
{
	DiffuseLobe diffuseLobe;
	SpecularLobe specularLobe;
	TransmissionLobe transmissionLobe;
};

float diffuseBsdfSampleWeight(DiffuseLobe lobe)
{
	return linearToLuma(lobe.albedo);
}

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

float specularBsdfSampleWeight(SpecularLobe lobe, float halfVecDotWin)
{
	return linearToLuma(fresnelSchlick(lobe.albedo, vec3(1.0), halfVecDotWin));
}

bool specularBsdfIsConductor(SpecularLobe lobe)
{
	return lobe.metallicFactor > 1.0 - METALLIC_THRESHOLD;
}

bool specularBsdfIsDielectric(SpecularLobe lobe)
{
	return lobe.metallicFactor < METALLIC_THRESHOLD;
}

vec3 specularBsdfFresnel(SpecularLobe lobe, float half_vec_dot_win, float ni, float nt)
{
    if (specularBsdfIsDielectric(lobe))
	{
        return vec3(fresnelDielectric(half_vec_dot_win, ni, nt));
    }
	else if (specularBsdfIsConductor(lobe))
	{
        return fresnelConductorFitted(half_vec_dot_win, ni);
    }
	else
	{
        vec3 f_dielectric = vec3(fresnelDielectric(half_vec_dot_win, ni, nt));
        vec3 f_conductor = fresnelConductorFitted(half_vec_dot_win, ni);
        return mix(f_dielectric, f_conductor, lobe.metallicFactor);
    }
}

BsdfSample sampleSpecularBsdf(SpecularLobe lobe, vec2 uv, vec3 w_out_tangent_space)
{
    float ggx_alpha = getGgxAlpha(lobe.roughnessFactor);
    vec3 microfacet_normal_tangent_space = sampleGgxVndf(w_out_tangent_space, ggx_alpha, uv);
    
    BsdfSample bsdfSample;
    bsdfSample.refract = false;
    bsdfSample.wInTangentSpace = _reflect(-w_out_tangent_space, microfacet_normal_tangent_space);
    
    if (bsdfSample.wInTangentSpace.z > MIN_COS_THETA_THRESHOLD)
	{
        bsdfSample.pdf = evalGgxVndfPdf(ggx_alpha, w_out_tangent_space, microfacet_normal_tangent_space);
    }
    
    return bsdfSample;
}

BsdfEval evalSpecularBsdf(SpecularLobe lobe, vec3 w_out_tangent_space, vec3 w_in_tangent_space)
{
    float ggx_alpha = getGgxAlpha(lobe.roughnessFactor);
    vec3 half_vec_tangent_space = normalize(w_out_tangent_space + w_in_tangent_space);
    vec3 f = specularBsdfFresnel(lobe, dot(half_vec_tangent_space, w_in_tangent_space), IOR_AIR, lobe.ior);
    float g2 = evalGgxG2(w_in_tangent_space.z, w_out_tangent_space.z, ggx_alpha);
    float jacobian = 4.0 * w_in_tangent_space.z * w_out_tangent_space.z;

    BsdfEval eval;
    eval.reflectance = (evalGgxNdf(ggx_alpha, half_vec_tangent_space) * f * g2) / jacobian;
    eval.fractionTransmitted = 1.0 - f;

    // Conductor base color hack
    if (specularBsdfIsConductor(lobe))
    {
        eval.reflectance *= lobe.albedo;
    }

    return eval;
}

vec3 specularBsdfTirCompensationFactor(SpecularLobe lobe, vec3 w_out_tangent_space, vec3 w_in_tangent_space)
{
    float ggx_alpha = getGgxAlpha(lobe.roughnessFactor);
    float g2 = evalGgxG2(w_in_tangent_space.z, w_out_tangent_space.z, ggx_alpha);
    vec3 half_vec_tangent_space = normalize(w_out_tangent_space + w_in_tangent_space);
    vec3 f = specularBsdfFresnel(lobe, dot(half_vec_tangent_space, w_in_tangent_space), lobe.ior, IOR_AIR);
    return (1.0 - g2) + (1.0 - f) * g2;
}

vec3 evalSpecularBsdfVarnishTransmittance(SpecularLobe lobe, vec3 w_out_tangent_space, vec3 w_in_tangent_space)
{
    float relative_ior = lobe.ior / IOR_AIR; // TODO: precompute 1.0 / IOR_AIR
    vec3 w_in_tangent_space_refract = refractDir(w_in_tangent_space, relative_ior);
    vec3 w_out_tangent_space_refract = refractDir(w_out_tangent_space, relative_ior);

    bool tir = w_in_tangent_space_refract.z < MIN_COS_THETA_THRESHOLD || w_out_tangent_space_refract.z < MIN_COS_THETA_THRESHOLD;
    if (tir)
    {
        return vec3(0.0);
    }

    vec3 absorption = calculateAbsorption(lobe.albedo, lobe.thickness, w_out_tangent_space_refract.z, w_in_tangent_space_refract.z);
    vec3 tir_factor = specularBsdfTirCompensationFactor(lobe, w_out_tangent_space_refract, w_in_tangent_space_refract);
    return absorption * tir_factor;
}

BsdfSample sampleTransmissionBsdf(TransmissionLobe lobe, float u, vec3 w_out_tangent_space, bool front_face)
{
    float ni = front_face ? IOR_AIR : lobe.ior;
    float nt = !front_face ? IOR_AIR : lobe.ior;
    float r = fresnelDielectric(w_out_tangent_space.z, ni, nt);
    float t = 1.0 - r;

    BsdfSample bsdfSample;
    bsdfSample.refract = u > r;
    if (!bsdfSample.refract)
    {
        bsdfSample.wInTangentSpace = reflect(-w_out_tangent_space, vec3(0.0, 0.0, 1.0));
        bsdfSample.pdf = r;
    }
    else
    {
        bsdfSample.wInTangentSpace = refract(-w_out_tangent_space, vec3(0.0, 0.0, 1.0), ni / nt);
        bsdfSample.pdf = t;
    }
    return bsdfSample;
}

BsdfEval evalTransmissionBsdf(TransmissionLobe lobe, vec3 w_out_tangent_space, vec3 w_in_tangent_space, bool front_face)
{
    float ni = front_face ? IOR_AIR : lobe.ior;
    float nt = !front_face ? IOR_AIR : lobe.ior;
    float r = fresnelDielectric(w_out_tangent_space.z, ni, nt);
    float t = 1.0 - r;

    bool refract = w_out_tangent_space.z * w_in_tangent_space.z <= 0.0;

    BsdfEval eval;
    if (!refract)
    {
        eval.reflectance = vec3(r / abs(w_in_tangent_space.z));
    }
    else
    {
        eval.reflectance = vec3((t / abs(w_in_tangent_space.z)) / sqr(ni / nt));
    }
    return eval;
}

bool layeredBsdfHasDiffuseLobe(LayeredLobe lobe)
{
    return lobe.diffuseLobe.albedo != vec3(-1.0);
}

bool layeredBsdfHasTransmissionLobe(LayeredLobe lobe)
{
    return lobe.transmissionLobe.ior != 0.0;
}

BsdfSample sampleLayeredBsdf(LayeredLobe lobe, vec3 uvw, vec3 w_out_tangent_space, bool front_face)
{
    if (layeredBsdfHasTransmissionLobe(lobe))
    {
        return sampleTransmissionBsdf(lobe.transmissionLobe, uvw.x, w_out_tangent_space, front_face);
    }

    BsdfSample bsdfSample = sampleSpecularBsdf(lobe.specularLobe, uvw.xy, w_out_tangent_space);
    float diffuse_weight = 0.0;
    if (layeredBsdfHasDiffuseLobe(lobe))
    {
        diffuse_weight = diffuseBsdfSampleWeight(lobe.diffuseLobe);
    }

    vec3 half_vec_tangent_space = normalize(w_out_tangent_space + bsdfSample.wInTangentSpace);
    float specular_weight = specularBsdfSampleWeight(lobe.specularLobe, dot(half_vec_tangent_space, bsdfSample.wInTangentSpace));

    float total = diffuse_weight + specular_weight;
    if (total == 0.0)
    {
        return BsdfSample(vec3(0.0), 0.0, false);
    }

    float diffuse_layer_pdf = diffuse_weight / total;
    if (uvw.z < diffuse_layer_pdf)
    {
        bsdfSample = sampleDiffuseBsdf(lobe.diffuseLobe, uvw.xy);
        bsdfSample.pdf *= diffuse_layer_pdf;
    }
    else
    {
        float specular_layer_pdf = specular_weight / total;
        bsdfSample.pdf *= specular_layer_pdf;
    }
    return bsdfSample;
}

BsdfEval evalLayeredBsdf(LayeredLobe lobe, vec3 w_out_tangent_space, vec3 w_in_tangent_space, bool front_face)
{
    if (layeredBsdfHasTransmissionLobe(lobe))
    {
        return evalTransmissionBsdf(lobe.transmissionLobe, w_out_tangent_space, w_in_tangent_space, front_face);
    }

    BsdfEval eval;
    if (w_out_tangent_space.z <= 0.0 || w_in_tangent_space.z <= 0.0)
    {
        return eval;
    }

    BsdfEval specular_eval = evalSpecularBsdf(lobe.specularLobe, w_out_tangent_space, w_in_tangent_space);
    vec3 varnish_attenuation = evalSpecularBsdfVarnishTransmittance(lobe.specularLobe, w_out_tangent_space, w_in_tangent_space);
    
    vec3 fraction_transmitted = specular_eval.fractionTransmitted * varnish_attenuation;

    vec3 diffuse_reflectance = vec3(0.0);
    if (layeredBsdfHasDiffuseLobe(lobe) && dot(fraction_transmitted, fraction_transmitted) > SQUARED_ABSORPTION_THRESHOLD)
    {
        diffuse_reflectance = evalDiffuseBsdf(lobe.diffuseLobe).reflectance * fraction_transmitted;
    }

    eval.reflectance = diffuse_reflectance + specular_eval.reflectance;
    eval.fractionTransmitted = vec3(0.0);
    return eval;
}

#endif // LAYERED_LOBE_H