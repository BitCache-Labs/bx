#include "[engine]/shaders/Language.shader"

#define MATERIAL_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS

#include "[engine]/shaders/ray_tracing/material.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/ray_tracing/sky.shader"

#include "[engine]/shaders/wfpt/payload.shader"
#include "[engine]/shaders/wfpt/sampling.shader"
#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/ray_tracing/sample.shader"
#include "[engine]/shaders/restir/restir.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint width;
    uint height;
    uint bounce;
    uint seed;
    bool russianRoulette;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (BINDING(0, 1), std430) readonly buffer _Rays
{
    PackedRay rays[];
};
layout (BINDING(0, 2), std430) writeonly buffer _OutRays
{
    PackedRay outRays[];
};

layout(BINDING(0, 3), std430) readonly buffer _RayCount
{
    uint rayCount;
};
layout(BINDING(0, 4), std430) buffer _OutRayCount
{
    uint outRayCount;
};

layout (BINDING(0, 5), std430) readonly buffer _PixelMapping
{
    uint pixelMapping[];
};
layout (BINDING(0, 6), std430) writeonly buffer _OutPixelMapping
{
    uint outPixelMapping[];
};

layout (BINDING(0, 7), std430) buffer _Payloads
{
    Payload payloads[];
};

layout (BINDING(0, 8), std430) readonly buffer _Intersections
{
    Intersection intersections[];
};

layout (BINDING(0, 9), std430) writeonly buffer _ShadowRays
{
    PackedRay shadowRays[];
};
layout(BINDING(0, 10), std430) writeonly buffer _ShadowRayDistances
{
    float shadowRayDistances[];
};
layout(BINDING(0, 11), std430) buffer _ShadowRayCount
{
    uint shadowRayCount;
};
layout(BINDING(0, 12), std430) writeonly buffer _ShadowPixelMapping
{
    uint shadowPixelMapping[];
};

Sample sampleUniformLight(vec4 random, vec3 p)
{
    uint emissiveTriangleCount = blasDataConstants.emissiveTriangleCount;

    float sunPickProbability = 0.0;//emissiveTriangleCount == 0 ? 1.0 : 0.5;
    if (random.x < sunPickProbability)
    {
        Sample lightSample;
        lightSample.directionToLight = sampleSunDirection(random.yz);
        lightSample.distanceToLight = 10000.0;
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
            vec3 barycentrics = barycentricsFromUv(random.zw);
            vec3 samplePosition = triangle.p0 * barycentrics.x + triangle.p1 * barycentrics.y + triangle.p2 * barycentrics.z;

            vec3 directionToLight = samplePosition - p;
            float distanceToLight = length(directionToLight);
            directionToLight = normalize(directionToLight);

            float pdf = (distanceToLight * distanceToLight) / float(emissiveTriangleCount);
            pdf *= (1.0 - sunPickProbability);

            Sample lightSample;
            lightSample.directionToLight = directionToLight;
            lightSample.distanceToLight = distanceToLight;
            lightSample.pdf = pdf;
            return lightSample;
        }
        else
        {
            offset += blas.triangleCount;
        }
    }
}

void shootRay(vec3 origin, vec3 direction, uint pid)
{
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;

    uint rayIdx = atomicAdd(outRayCount, 1u);
    outRays[rayIdx] = packRay(ray);
    outPixelMapping[rayIdx] = pid;
}

void shootShadowRay(vec3 origin, vec3 direction, float tMax, uint pid)
{
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;

    uint rayIdx = atomicAdd(shadowRayCount, 1u);
    shadowRays[rayIdx] = packRay(ray);
    shadowRayDistances[rayIdx] = tMax;
    shadowPixelMapping[rayIdx] = pid;
}

vec3 shadeSky(vec3 direction, vec3 throughput)
{
    float a = (direction.y + 1.0) * 0.5;
    vec3 color = vec3(0.0);//(1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);

    return color * throughput;
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= rayCount) return;
    uint pid = pixelMapping[id];

    Intersection intersection = intersections[pid];
    Payload payload = payloads[pid];
    Ray ray = unpackRay(rays[id]);

    vec3 throughput = unpackRgb9e5(payload.throughput);
    vec3 accumulated = unpackRgb9e5(payload.accumulated);
    if (constants.bounce == 0)
    {
        accumulated = vec3(0.0);
        throughput = vec3(1.0);
        payload.rngState = pcgHash(pid ^ xorShiftU32(constants.seed));
    }

    if (intersection.t != T_MISS)
    {
        // Query vertex data from global blas data pool
        BlasInstance blasInstance = blasInstances[intersection.blasInstanceIdx];
        BlasAccessor blasAccessor = blasAccessors[blasInstance.blasIdx];

        Triangle triangle = blasTriangles[intersection.primitiveIdx + blasAccessor.triangleOffset];
        PackedVertex vertex0 = blasVertices[triangle.i0 + blasAccessor.vertexOffset];
        PackedVertex vertex1 = blasVertices[triangle.i1 + blasAccessor.vertexOffset];
        PackedVertex vertex2 = blasVertices[triangle.i2 + blasAccessor.vertexOffset];

        vec3 normal0 = unpackNormalizedXyz10(vertex0.normal, 0);
        vec3 normal1 = unpackNormalizedXyz10(vertex1.normal, 0);
        vec3 normal2 = unpackNormalizedXyz10(vertex2.normal, 0);
        vec2 texCoord0 = unpackHalf2x16(vertex0.texCoord);
        vec2 texCoord1 = unpackHalf2x16(vertex1.texCoord);
        vec2 texCoord2 = unpackHalf2x16(vertex2.texCoord);

        vec3 barycentrics = barycentricsFromUv(intersection.uv);
        vec3 normal = normalize(normal0 * barycentrics.x
            + normal1 * barycentrics.y
            + normal2 * barycentrics.z);
        vec2 texCoord = texCoord0 * barycentrics.x
            + texCoord1 * barycentrics.y
            + texCoord2 * barycentrics.z;

        // Correct normal for transform and backface hits
        mat4 invTransTransform = transpose(blasInstance.invTransform);
        normal = normalize((invTransTransform * vec4(normal, 1.0)).xyz);
        if (!intersection.frontFace)
        {
            normal = -normal;
        }

        // Build tangent to world matrix and correct for wOut below hemisphere (can happen due to normal mapping)
        mat3 tangentToWorld = buildOrthonormalBasis(normal);
        mat3 worldToTangent = transpose(tangentToWorld);

        vec3 wOutTangentSpace = normalize(worldToTangent * -ray.direction);
        if (wOutTangentSpace.z < 0.0 && intersection.frontFace)
        {
            wOutTangentSpace.z *= -0.25;
            wOutTangentSpace = normalize(wOutTangentSpace);
        }

        // Load and sample material
        SampledMaterial material = sampleMaterial(materialDescriptors[blasInstance.materialIdx], texCoord);

        vec3 bsdfNoise = randomUniformFloat3(payload.rngState);

        LayeredLobe layeredLobe = layeredLobeFromMaterial(material);
        BsdfSample bsdfSample = sampleLayeredBsdf(layeredLobe, bsdfNoise, wOutTangentSpace, intersection.frontFace);
        BsdfEval bsdfEval = evalLayeredBsdf(layeredLobe, wOutTangentSpace, bsdfSample.wInTangentSpace, intersection.frontFace);

        vec3 wInWorldSpace = vec3(0.0);
        bool validSample = applyBsdf(bsdfSample, bsdfEval, tangentToWorld, normal, throughput, wInWorldSpace);

        vec3 intersectionPos = ray.origin + ray.direction * intersection.t;

        { // Direct illumination
#if 1
            RestirSample lightSample = generateRestirSample(payload.rngState,
                layeredLobe, worldToTangent, tangentToWorld,
                normal, intersection.frontFace,
                intersectionPos, ray.origin);

            vec3 directionToLight = normalize(lightSample.path.x2 - lightSample.path.x1);
            float distanceToLight = distance(lightSample.path.x2, lightSample.path.x1);

            if (dot(directionToLight, normal) > 0.0)
            {
                vec3 origin = intersectionPos;
                vec3 direction = directionToLight;
                origin += direction * RT_EPSILON;
                
                payload.directIlluminationPdf = 1.0 / lightSample.weight;
                
                shootShadowRay(origin, direction, distanceToLight, pid);
            }
#else
            Sample lightSample = sampleUniformLight(randomUniformFloat4(payload.rngState), intersectionPos);
            
            if (dot(lightSample.directionToLight, normal) > 0.0)
            {
                vec3 origin = intersectionPos;
                vec3 direction = lightSample.directionToLight;
                origin += direction * RT_EPSILON;
                
                payload.directIlluminationPdf = lightSample.pdf;
                
                shootShadowRay(origin, direction, lightSample.distanceToLight, pid);
            }
#endif
        }

        // Indirect illumination
        if (validSample)
        {
            bool russianRouletteTerminate = false;
            if (constants.russianRoulette && constants.bounce > 1)
            {
                float russianRoulette = max(throughput.r, max(throughput.g, throughput.b));
                russianRouletteTerminate = randomUniformFloat(payload.rngState) > russianRoulette;

                if (russianRouletteTerminate)
                {
                    throughput *= 1.0 / russianRoulette;
                }
            }

            vec3 origin = intersectionPos;
            vec3 direction = wInWorldSpace;
            origin += direction * RT_EPSILON;
            shootRay(origin, direction, pid);
        }
    }
    else
    {
        accumulated += shadeSky(ray.direction, throughput);
    }

    payload.throughput = packRgb9e5(throughput);
    payload.accumulated = packRgb9e5(accumulated);
    payloads[pid] = payload;
}