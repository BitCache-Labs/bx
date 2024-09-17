#ifndef WFPT_SAMPLING_H
#define WFPT_SAMPLING_H

#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/restir/restir.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/color_helpers.shader"

struct LightSample
{
    vec3 sampleDirection;
    float hitT;
    float pdf;
};

struct RisResult
{
    ReservoirData reservoirData;
    Reservoir reservoir;
};

LightSample _sampleUniformLight(vec4 random, vec3 p)
{
    uint emissiveTriangleCount = blasDataConstants.emissiveTriangleCount;

    float sunPickProbability = 0.0;//emissiveTriangleCount == 0 ? 1.0 : 0.5;
    if (random.x < sunPickProbability)
    {
        LightSample lightSample;
        lightSample.sampleDirection = sampleSunDirection(random.yz);
        lightSample.hitT = 10000.0;
        lightSample.pdf = sunPickProbability;//sunPickProbability * (1.0 / sunSolidAngle());
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
            uint vertexOffset = blas.vertexOffset;

            Triangle triangle = transformedTriangle(blasTriangles[triangleIndex + triangleOffset], inverse(instance.invTransform));
            vec3 edge1 = triangle.p1 - triangle.p0;
            vec3 edge2 = triangle.p2 - triangle.p0;
            float area = calculateTriangleAreaFromEdges(edge1, edge2);
            vec3 triangleNormal = normalize(cross(edge1, edge2));
            vec3 barycentrics = barycentricsFromUv(random.zw);
            vec3 samplePosition = triangle.p0 * barycentrics.x + triangle.p1 * barycentrics.y + triangle.p2 * barycentrics.z;

            vec3 directionToLight = samplePosition - p;
            float distanceToLight = length(directionToLight);
            directionToLight = normalize(directionToLight);

            float cosOut = abs(dot(triangleNormal, -directionToLight));

            float pdf = 1.0 / (triangleLightSolidAngle(cosOut, area, distanceToLight));
            pdf *= 1.0 / float(emissiveTriangleCount);
            pdf *= (1.0 - sunPickProbability);

            LightSample lightSample;
            lightSample.sampleDirection = directionToLight;
            lightSample.hitT = distanceToLight;
            lightSample.pdf = pdf;
            return lightSample;
        }
        else
        {
            offset += blas.triangleCount;
        }
    }

    return LightSample(vec3(0.0), 0.0, 0.0);
}

RisResult ris(inout uint rngState,
    LayeredLobe layeredLobe, mat3 worldToTangent, mat3 tangentToWorld,
    vec3 normal, bool frontFace,
    vec3 x1, vec3 x0)
{
#if 1
    LightSample lightSample = _sampleUniformLight(randomUniformFloat4(rngState), x1);

    ReservoirData reservoirData;
    reservoirData.sampleDirection = lightSample.sampleDirection;
    reservoirData.hitT = lightSample.hitT;
    Reservoir reservoir = Reservoir_default();
    reservoir.contributionWeight = 1.0 / lightSample.pdf;
    return RisResult(reservoirData, reservoir);
#else

    const uint M_AREA = 32;

    vec3 wOutWorldSpace = normalize(x1 - x0);
    vec3 wOutTangentSpace = normalize(worldToTangent * wOutWorldSpace);

	ReservoirData reservoirData;
    Reservoir reservoir = Reservoir_default();

    #pragma unroll
	for (uint i = 0; i < M_AREA; i++)
	{
        LightSample lightSample = _sampleUniformLight(randomUniformFloat4(rngState), x1);
    
        vec3 wInWorldSpace = lightSample.sampleDirection;
        vec3 wInTangentSpace = normalize(worldToTangent * wInWorldSpace);
    
        float unoccludedContributionWeight = 0.0;
        if (dot(wInWorldSpace, normal) > 0.0)
        {
            BsdfEval bsdfEval = evalLayeredBsdf(layeredLobe, wOutTangentSpace, wInTangentSpace, frontFace);
            vec3 bsdfContribution = bsdfContribution(bsdfEval, normal, wInWorldSpace, 1.0);
            unoccludedContributionWeight = fixNan(linearToLuma(bsdfContribution));
        }

        float contributionWeight = 1.0 / lightSample.pdf;
        float weight = unoccludedContributionWeight * contributionWeight;
        if (Reservoir_update(reservoir, weight, rngState))
        {
            reservoirData = ReservoirData(lightSample.sampleDirection, lightSample.hitT);
            reservoir.contributionWeight = contributionWeight;
        }
	}
    
    //reservoir.weight = (reservoir.outputSample.unoccludedContributionWeight == 0.0) ? 0.0 : (1.0 / reservoir.outputSample.unoccludedContributionWeight);
    //reservoir.weight *= ((reservoir.sampleCount == 0) ? 0.0 : (1.0 / reservoir.sampleCount)) * reservoir.weightSum;
    //
    //reservoir.weight = fixNan(reservoir.weight);

    return RisResult(reservoirData, reservoir);
#endif
}

#endif // WFPT_SAMPLING_H