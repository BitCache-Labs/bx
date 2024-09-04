#ifndef WFPT_SAMPLING_H
#define WFPT_SAMPLING_H

#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/restir/restir.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/color_helpers.shader"

RestirSample _sampleUniformLight(vec4 random, vec3 p)
{
    uint emissiveTriangleCount = blasDataConstants.emissiveTriangleCount;

    float sunPickProbability = 0.0;//emissiveTriangleCount == 0 ? 1.0 : 0.5;
    if (random.x < sunPickProbability)
    {
        RestirSample lightSample;
        lightSample.x1 = p;
        lightSample.x2 = p + sampleSunDirection(random.yz) * 1000.0;
        lightSample.weight = 1.0 / sunPickProbability;//sunPickProbability * (1.0 / sunSolidAngle());
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

            float pdf = 1.0 / (triangleLightSolidAngle(cosOut, area, distanceToLight) * float(emissiveTriangleCount));
            pdf *= (1.0 - sunPickProbability);

            RestirSample lightSample;
            lightSample._PADDING0 = 0;// Remove
            lightSample.x1 = p;
            lightSample.x2 = samplePosition;
            lightSample.weight = 1.0 / pdf;
            return lightSample;
        }
        else
        {
            offset += blas.triangleCount;
        }
    }

    RestirSample lightSample;
    lightSample.x1 = vec3(-1.0);
    lightSample.x2 = vec3(-1.0);
    lightSample.weight = -1.0;
    return lightSample;
}

RestirSample generateRestirSample(inout uint rngState,
    LayeredLobe layeredLobe, mat3 worldToTangent, mat3 tangentToWorld,
    vec3 normal, bool frontFace,
    vec3 x1, vec3 x0)
{
    const uint M_AREA = 32;
    const uint M_BSDF = 0;

    vec3 wOutWorldSpace = normalize(x1 - x0);
    vec3 wOutTangentSpace = normalize(worldToTangent * wOutWorldSpace);

	Reservoir reservoir = makeReservoir();

    #pragma unroll
	for (uint i = 0; i < M_AREA; i++)
	{
        RestirSample lightSample = _sampleUniformLight(randomUniformFloat4(rngState), x1);
        lightSample.x0 = x0;

        vec3 wInWorldSpace = normalize(lightSample.x2 - lightSample.x1);
        vec3 wInTangentSpace = normalize(worldToTangent * wInWorldSpace);

        BsdfEval bsdfEval = evalLayeredBsdf(layeredLobe, wOutTangentSpace, wInTangentSpace, frontFace);
        vec3 bsdfContribution = bsdfContribution(bsdfEval, normal, wInWorldSpace, 1.0); // 1.0 / lightSample.weight, but don't use pdf?
        lightSample.unoccludedContributionWeight = linearToLuma(bsdfContribution);
        
        if (i == 0)
        {
            reservoir.outputSample = lightSample;
        }

        float weight = (1.0 / (M_AREA + M_BSDF)) * lightSample.unoccludedContributionWeight * lightSample.weight;
        updateReservoir(reservoir, rngState, lightSample, weight);
	}

    //#pragma unroll
	//for (uint i = 0; i < M_BSDF; i++)
	//{
    //    vec3 wOutWorldSpace = normalize(x1 - x0);
    //    vec3 wOutTangentSpace = normalize(worldToTangent * wOutWorldSpace);
    //
    //    BsdfSample bsdfSample = sampleLayeredBsdf(layeredLobe, randomUniformFloat3(rngState), wOutTangentSpace, frontFace);
    //    //RestirSample lightSample = _sampleUniformLight(randomUniformFloat4(rngState), x1);
    //
    //    vec3 wInWorldSpace = normalize(tangentToWorld * bsdfSample.wInTangentSpace);
    //
    //    RestirSample lightSample;
    //    lightSample.path.x0 = x0;
    //    lightSample.path.x1 = x1;
    //    lightSample.path.x2 = x1 + wInWorldSpace * 1000.0;
    //    lightSample.weight = 1.0 / bsdfSample.pdf;
    //
    //    BsdfEval bsdfEval = evalLayeredBsdf(layeredLobe, wOutTangentSpace, bsdfSample.wInTangentSpace, frontFace);
    //    vec3 bsdfContribution = bsdfContribution(bsdfEval, normal, wInWorldSpace, 1.0 / lightSample.weight);
    //    float unoccludedContributionWeight = linearToLuma(bsdfContribution);
    //
    //    float weight = (1.0 / (M_AREA + M_BSDF)) * unoccludedContributionWeight * lightSample.weight;
    //    updateReservoir(reservoir, rngState, lightSample, weight);
	//}

    RestirSample outputSample = reservoir.outputSample;
    outputSample.weight = (1.0 / outputSample.unoccludedContributionWeight) * reservoir.weightSum;

    return outputSample;
}

#endif // WFPT_SAMPLING_H