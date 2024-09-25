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
};

struct RisResult
{
    ReservoirData reservoirData;
    Reservoir reservoir;
};

float triangleLightIntensity(uint triangleIndex, uint blasInstanceIdx, vec3 directionToLight, float distanceToLight)
{
    BlasInstance instance = blasInstances[blasInstanceIdx];
    mat4 transform = inverse(instance.invTransform);

    Triangle triangle = transformedTriangle(blasTriangles[triangleIndex], transform);
    vec3 edge1 = triangle.p1 - triangle.p0;
    vec3 edge2 = triangle.p2 - triangle.p0;

    float area = calculateTriangleAreaFromEdges(edge1, edge2);
    vec3 triangleNormal = normalize(cross(edge1, edge2));
    float cosOut = abs(dot(triangleNormal, -directionToLight));

    // TODO: incorporate material emissive factor

    return triangleLightSolidAngle(cosOut, area, distanceToLight) * 4.0;
}

float lightIntensity(uint triangleIndex, uint blasInstanceIdx, vec3 directionToLight, float distanceToLight)
{
    if (triangleIndex != U32_MAX && false)
    {
        return triangleLightIntensity(triangleIndex, blasInstanceIdx, directionToLight, distanceToLight);
    }
    else
    {
        return sunIntensity(directionToLight.y);
    }
}

LightSample sampleTriangleLight(uint triangleIndex, vec2 uv, mat4 transform, vec3 p, float sunPickProbability)
{
    uint emissiveTriangleCount = blasDataConstants.emissiveTriangleCount;

    Triangle triangle = transformedTriangle(blasTriangles[triangleIndex], transform);
    vec3 edge1 = triangle.p1 - triangle.p0;
    vec3 edge2 = triangle.p2 - triangle.p0;
    vec3 barycentrics = barycentricsFromUv(uv);
    vec3 samplePosition = triangle.p0 * barycentrics.x + triangle.p1 * barycentrics.y + triangle.p2 * barycentrics.z;

    vec3 directionToLight = samplePosition - p;
    float distanceToLight = length(directionToLight);
    directionToLight = normalize(directionToLight);

    float pdf = (1.0 - sunPickProbability) / float(emissiveTriangleCount);

    return LightSample(directionToLight, distanceToLight, pdf, triangleIndex, 0, uv);
}

LightSample _sampleUniformLight(vec4 random, vec3 p)
{
    uint emissiveTriangleCount = blasDataConstants.emissiveTriangleCount;

    float sunPickProbability = 0.0;//(emissiveTriangleCount == 0) ? 1.0 : 0.5;
    if (random.x < sunPickProbability)
    {
        LightSample lightSample;
        lightSample.sampleDirection = sampleSunDirection(random.yz);
        lightSample.hitT = SUN_DISTANCE;
        lightSample.pdf = sunPickProbability;
        lightSample.triangle = U32_MAX;
        lightSample.blasInstance = 0;
        lightSample.uv = random.yz;
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
            return lightSample;
        }
        else
        {
            offset += blas.triangleCount;
        }
    }

    return LightSample(vec3(0.0), 0.0, 0.0, 0, 0, vec2(0.0));
}

#endif // WFPT_SAMPLING_H