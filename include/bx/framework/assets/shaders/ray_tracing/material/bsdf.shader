#ifndef BSDF_H
#define BSDF_H

#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/sampling.shader"

#include "[engine]/shaders/ray_tracing/material/tangent_space_trig_helpers.shader"

const float MIN_ROUGHNESS_THRESHOLD = 0.08;
const float MIN_COS_THETA_THRESHOLD = 1e-6;
const float MIN_BRDF_PDF_THRESHOLD = 1e-9;
const float METALLIC_THRESHOLD = 1e-3f;
const float SQUARED_ABSORPTION_THRESHOLD = 1e-6;

const float IOR_AIR = 1.000293;

const uint _FE[16] = uint[](
	1195132717, 3295168244, 3319579794, 3112876607,
    1127038013, 1100497709, 3364538495, 1155975142,
    1147356411, 939276304, 3051632581, 3051009512,
    781201040, 11983, 0, 0
);

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

bool bsdfSampleValid(BsdfSample bsdfSample)
{
	return (bsdfSample.wInTangentSpace.z > MIN_COS_THETA_THRESHOLD || bsdfSample.refract) && bsdfSample.pdf > MIN_BRDF_PDF_THRESHOLD;
}

bool applyBsdf(BsdfSample bsdfSample, BsdfEval bsdfEval,
	mat3 tangentToWorld, vec3 normal,
	inout vec3 throughput, out vec3 wInWorldSpace)
{
	// TODO: possible optimization, can we skip eval and check if sample is valid earlier??
	if (!bsdfSampleValid(bsdfSample))
	{
		return false;
	}

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

vec3 refractDir(vec3 wInTangentSpace, float relativeIor)
{
	return -_refract(-wInTangentSpace, FORWARD, 1.0 / relativeIor);
}

vec3 calculateAbsorption(vec3 layer_color, float thickness, float cos_theta_wo, float cos_theta_wi)
{
    vec3 inverse_color = 1.0 - layer_color;
    float inverse_cos_theta_i = 1.0 / min(1.0, max(MIN_COS_THETA_THRESHOLD, cos_theta_wi));
    float inverse_cos_theta_o = 1.0 / min(1.0, max(MIN_COS_THETA_THRESHOLD, cos_theta_wo));
    float val = thickness * (inverse_cos_theta_i + inverse_cos_theta_o);
    vec3 absorption = exp(-inverse_color * val);
    return absorption;
}

float getGgxAlpha(float roughnessFactor)
{
	float roughness = max(MIN_ROUGHNESS_THRESHOLD, roughnessFactor);
	return sqr(roughness);
}

vec3 fresnelSchlickBasic(float wDotMicrofacetN)
{
	vec3 f0 = vec3(0.04);
	return f0 + (1.0 - f0) * pow(max(0.0, 1.0 - wDotMicrofacetN), 5.0);
}

vec3 fresnelSchlick(vec3 f0, vec3 f90, float wDotMicrofacetN)
{
    return f0 + (f90 - f0) * pow(max(0.0, 1.0 - wDotMicrofacetN), 5.0);
}

float fresnelDielectric(float cosTheta, float ni, float nt)
{
    float nt2 = sqr(nt);
    float ni2 = sqr(ni);
    float cosTheta2 = sqr(cosTheta);

    float sinTheta2 = 1.0 - cosTheta2;
    float sinTheta = safeSqrt(sinTheta2);
    float tanTheta = sinTheta / cosTheta;
    float tanTheta2 = sqr(tanTheta);

    float sq = nt2 - ni2 * sinTheta2;
    float sqr = sqrt(sqr(sq));
    float oneOver2NiSq = 1.0 / (2.0 * ni2);
    float ni2_sinTheta2 = ni2 * sinTheta2;

    float a2 = oneOver2NiSq * (sqr + (nt2 - ni2_sinTheta2));
    float b2 = oneOver2NiSq * (sqr - (nt2 - ni2_sinTheta2));
    float a = sqrt(a2);
    float ax2 = a * 2.0;

    float a2PlusB2 = a2 + b2;

    float rsNumer = a2PlusB2 - ax2 * cosTheta + cosTheta2;
    float rsDenom = a2PlusB2 + ax2 * cosTheta + cosTheta2;
    float rs = rsNumer / rsDenom;

    float rpNumer = a2PlusB2 - (ax2 * sinTheta * tanTheta) + sinTheta2 * tanTheta2;
    float rpDenom = a2PlusB2 + (ax2 * sinTheta * tanTheta) + sinTheta2 * tanTheta2;
    float rp = rs * (rpNumer / rpDenom);

    return 0.5 * (rp + rs);
}

vec3 polynomialFit(float v, vec3 c0, vec3 c1, vec3 c2)
{
    return c2 * pow(v, 2.0) + c1 * v + c0;
}

vec3 fresnelConductorFitted(float cosTheta, float ni)
{
    float ni01 = (min(ni, 2.5) - 1.0) / 1.5;

    vec2 unpacked0 = unpackHalf2x16(_FE[0 * 4 + 0]);
    vec2 unpacked1 = unpackHalf2x16(_FE[0 * 4 + 1]);
    vec2 unpacked2 = unpackHalf2x16(_FE[0 * 4 + 2]);
    vec2 unpacked3 = unpackHalf2x16(_FE[0 * 4 + 3]);
    vec2 unpacked4 = unpackHalf2x16(_FE[1 * 4 + 0]);
    vec3 pwr = polynomialFit(ni01,
        vec3(unpacked0, unpacked1.x),
        vec3(unpacked1.y, unpacked2),
        vec3(unpacked3, unpacked4.x)
    );
    
    unpacked0 = unpacked4;
    unpacked1 = unpackHalf2x16(_FE[1 * 4 + 1]);
    unpacked2 = unpackHalf2x16(_FE[1 * 4 + 2]);
    unpacked3 = unpackHalf2x16(_FE[1 * 4 + 3]);
    unpacked4 = unpackHalf2x16(_FE[2 * 4 + 0]);
    vec3 a = polynomialFit(ni01,
        vec3(unpacked0.y, unpacked1),
        vec3(unpacked2, unpacked3.x),
        vec3(unpacked3.y, unpacked4)
    );
    
    unpacked0 = unpackHalf2x16(_FE[2 * 4 + 1]);
    unpacked1 = unpackHalf2x16(_FE[2 * 4 + 2]);
    unpacked2 = unpackHalf2x16(_FE[2 * 4 + 3]);
    unpacked3 = unpackHalf2x16(_FE[3 * 4 + 0]);
    unpacked4 = unpackHalf2x16(_FE[3 * 4 + 1]);
    vec3 f0 = polynomialFit(ni01,
        vec3(unpacked0, unpacked1.x),
        vec3(unpacked1.y, unpacked2),
        vec3(unpacked3, unpacked4.x)
    );
    
    vec3 v = a * cosTheta * pow(vec3(1.0 - cosTheta), pwr);
    
    return saturate(f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0) - v);
}

float evalGgxG1(float nDotWo, float ggxAlpha)
{
    if (nDotWo <= 0.0)
    {
        return 0.0;
    }
    float a2 = sqr(ggxAlpha);
    float denomC = sqrt(a2 + (1.0 - a2) * sqr(nDotWo)) + nDotWo;
    return (2.0 * nDotWo) / denomC;
}

float evalGgxG2(float nDotWi, float nDotWo, float ggxAlpha)
{
    float a2 = sqr(ggxAlpha);
    float denomA = nDotWo * sqrt(a2 + (1.0 - a2) * sqr(nDotWi));
    float denomB = nDotWi * sqrt(a2 + (1.0 - a2) * sqr(nDotWo));
    return (2.0 * nDotWi * nDotWo) / (denomA + denomB);
}

vec3 sampleGgxVndf(vec3 wOutTangentSpace, float ggxAlpha, vec2 uv)
{
    vec2 alpha = vec2(ggxAlpha);

    vec3 wOutHemisphere = normalize(vec3(
        wOutTangentSpace.x * alpha.x,
        wOutTangentSpace.y * alpha.y,
        wOutTangentSpace.z
    ));

    float phi = TWO_PI * uv.x;
    float a = saturate(min(alpha.x, alpha.y));
    float s = 1.0 + length(wOutTangentSpace.xy);
    float a2 = sqr(a);
    float s2 = sqr(s);
    float k = (1.0 - a2) * s2 / (s2 + a2 * sqr(wOutTangentSpace.z));
    float b;
    if (wOutTangentSpace.z > 0.0) { // TODO: use mix
        b = k * wOutHemisphere.z;
    } else {
        b = wOutHemisphere.z;
    }

    float z = (1.0 - uv.y) * (1.0 + b) - b;
    float sinTheta = safeSqrt(saturate(1.0 - sqr(z)));
    vec3 wInHemisphere = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    vec3 microfacetHemisphere = wOutHemisphere + wInHemisphere;

    return normalize(vec3(
        microfacetHemisphere.xy * alpha,
        max(0.0, microfacetHemisphere.z)
    ));
}

float evalGgxNdf(float ggxAlpha, vec3 microfacetNormal)
{
    if (microfacetNormal.z <= 0.0)
    {
        return 0.0;
    }

    float a2 = sqr(ggxAlpha);
    float cos2Theta;
    float squared_part_of_denom = (a2 + tan2ThetaTangentSpaceIntermediate(microfacetNormal, cos2Theta));
    return a2 / (PI * sqr(cos2Theta) * sqr(squared_part_of_denom));
}

float evalGgxVndfPdf(float ggxAlpha, vec3 wOutTangentSpace, vec3 microfacetNormal)
{
    vec2 alpha = vec2(ggxAlpha);
    float ndf = evalGgxNdf(ggxAlpha, microfacetNormal);
    vec2 a_o = alpha * wOutTangentSpace.xy;
    float len2 = dot(a_o, a_o);
    float t = safeSqrt(len2 + sqr(wOutTangentSpace.z));

    if (wOutTangentSpace.z > 0.0)
    {
        float a = saturate(min(alpha.x, alpha.y));
        float s = 1.0 + length(wOutTangentSpace.xy);
        float a2 = sqr(a);
        float s2 = sqr(s);
        float k = (1.0 - a2) * s2 / (s2 + a2 * sqr(wOutTangentSpace.z));
        return ndf / (2.0 * (k * wOutTangentSpace.z + t));
    }

    return ndf * (t - wOutTangentSpace.z) / (2.0 * len2);
}

#endif // BSDF_H