#ifndef WFPT_SAMPLING_H
#define WFPT_SAMPLING_H

#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/restir/restir.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/ray_tracing/material/bsdf.shader"
#include "[engine]/shaders/color_helpers.shader"

struct LightSample
{
    vec3 sampleDirection;
    float hitT;
    float pdf;
    uint triangle;
    uint blasInstance;
    vec2 uv;
    float intensity;
};

struct RisResult
{
    ReservoirData reservoirData;
    Reservoir reservoir;
};

LightSample sampleTriangleLight(uint triangleIndex, vec2 uv, mat4 transform, vec3 p, float sunPickProbability)
{
    uint emissiveTriangleCount = blasDataConstants.emissiveTriangleCount;

    Triangle triangle = transformedTriangle(blasTriangles[triangleIndex], transform);
    vec3 edge1 = triangle.p1 - triangle.p0;
    vec3 edge2 = triangle.p2 - triangle.p0;
    float area = calculateTriangleAreaFromEdges(edge1, edge2);
    vec3 triangleNormal = normalize(cross(edge1, edge2));
    vec3 barycentrics = barycentricsFromUv(uv);
    vec3 samplePosition = triangle.p0 * barycentrics.x + triangle.p1 * barycentrics.y + triangle.p2 * barycentrics.z;

    vec3 directionToLight = samplePosition - p;
    float distanceToLight = length(directionToLight);
    directionToLight = normalize(directionToLight);

    float cosOut = abs(dot(triangleNormal, -directionToLight));

    float pdf = 1.0 / (triangleLightSolidAngle(cosOut, area, distanceToLight));
    pdf *= 1.0 / float(emissiveTriangleCount);
    pdf *= (1.0 - sunPickProbability);

    return LightSample(directionToLight, distanceToLight, pdf, 0, 0, uv, 6.0);
}

float sunPdf(float sunPickProbability)
{
    return sunPickProbability * (1.0 / sunSolidAngle());
}

LightSample _sampleUniformLight(vec4 random, vec3 p)
{
    uint emissiveTriangleCount = blasDataConstants.emissiveTriangleCount;

    float sunPickProbability = emissiveTriangleCount == 0 ? 1.0 : 0.5;
    if (random.x < sunPickProbability)
    {
        LightSample lightSample;
        lightSample.sampleDirection = sampleSunDirection(random.yz);
        lightSample.hitT = 10000.0;
        lightSample.pdf = sunPdf(sunPickProbability);
        lightSample.triangle = U32_MAX;
        lightSample.intensity = sunIntensity(lightSample.sampleDirection.y);
        return lightSample;
    }

    uint instanceCount = blasDataConstants.emissiveInstanceCount;
    uint pickedTriangleIdx = uint(random.y * float(emissiveTriangleCount));

    uint offset = 0;
    for (uint i = 0; i < instanceCount; i++)
    {
        BlasInstance instance = blasInstances[blasEmissiveInstanceIndices[i]];
        BlasAccessor blas = blasAccessors[instance.blasIdx];

        if (pickedTriangleIdx < blas.triangleCount + offset)
        {
            uint triangleIndex = pickedTriangleIdx - offset;
            uint triangleOffset = blas.triangleOffset;
            triangleIndex += triangleOffset;

            LightSample lightSample = sampleTriangleLight(triangleIndex, random.zw, inverse(instance.invTransform), p, sunPickProbability);
            lightSample.triangle = triangleIndex;
            lightSample.blasInstance = blasEmissiveInstanceIndices[i];
            lightSample.intensity = 10.0;
            return lightSample;
        }
        else
        {
            offset += blas.triangleCount;
        }
    }

    return LightSample(vec3(0.0), 0.0, 0.0, 0, 0, vec2(0.0), 0.0);
}

RisResult ris(inout uint rngState,
    vec3 baseColor, mat3 worldToTangent, mat3 tangentToWorld,
    vec3 normal, bool frontFace,
    vec3 x1, vec3 x0)
{
#if 0
    LightSample lightSample = _sampleUniformLight(randomUniformFloat4(rngState), x1);

    ReservoirData reservoirData = ReservoirData(lightSample.sampleDirection, lightSample.hitT, lightSample.triangle,
        lightSample.blasInstance, lightSample.uv, 0.0);
    Reservoir reservoir = Reservoir_default();
    reservoir.contributionWeight = 1.0 / lightSample.pdf;
    reservoir.sampleCount = 1.0;
    return RisResult(reservoirData, reservoir);
#else

    const uint M_AREA = 8;

    vec3 wOutWorldSpace = normalize(x1 - x0);
    vec3 wOutTangentSpace = normalize(worldToTangent * wOutWorldSpace);

    ReservoirData reservoirData = ReservoirData(vec3(0.0), 0.0, 0, 0, vec2(0.0), 0.0);
    Reservoir reservoir = Reservoir_default();
	
    #pragma unroll
	for (uint i = 0; i < M_AREA; i++)
	{
        LightSample lightSample = _sampleUniformLight(randomUniformFloat4(rngState), x1);
        float contributionWeight = 1.0 / lightSample.pdf;

        vec3 wInWorldSpace = lightSample.sampleDirection;
        vec3 wInTangentSpace = normalize(worldToTangent * wInWorldSpace);
    
        float unoccludedContributionWeight = 0.0;
        if (dot(wInWorldSpace, normal) > 0.0)
        {
            //BsdfEval bsdfEval = evalLayeredBsdf(layeredLobe, wOutTangentSpace, wInTangentSpace, frontFace);
            //vec3 bsdfContribution = bsdfContribution(bsdfEval, normal, wInWorldSpace, 1.0);
            //
            vec3 brdfEval = diffuseBsdfEval(baseColor);
            vec3 brdfContribution = bsdfContribution(brdfEval, normal, wInWorldSpace, 1.0);
            unoccludedContributionWeight = fixNan(linearToLuma(brdfContribution)) * lightSample.intensity;
        }

        float weight = unoccludedContributionWeight * contributionWeight;
        if (Reservoir_update(reservoir, weight, rngState))
        {
            reservoirData = ReservoirData(lightSample.sampleDirection, lightSample.hitT, lightSample.triangle,
                lightSample.blasInstance, lightSample.uv, unoccludedContributionWeight);
        }
	}

    reservoir.contributionWeight = reservoir.weightSum / max(1e-8, reservoirData.unoccludedContributionWeight * reservoir.sampleCount);
    reservoir.contributionWeight = fixNan(reservoir.contributionWeight);

    return RisResult(reservoirData, reservoir);
#endif
}

#endif // WFPT_SAMPLING_H