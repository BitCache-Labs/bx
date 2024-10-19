#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define MATERIAL_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS
#define RESTIR_BINDINGS

#include "[engine]/shaders/ray_tracing/material.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/ray_tracing/sky.shader"
#include "[engine]/shaders/passes/restir_di/restir.shader"

#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/ray_tracing/sample.shader"
#include "[engine]/shaders/ray_tracing/light_picking.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 globalResolution;
    uvec2 resolution;
    uint seed;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (BINDING(0, 1), std430) readonly buffer _Rays
{
    PackedRay rays[];
};

layout (BINDING(0, 2), std430) readonly buffer _Intersections
{
    Intersection intersections[];
};

layout(BINDING(0, 3), std430) readonly buffer _SampleCount
{
    uint sampleCount;
};
layout(BINDING(0, 4), std430) readonly buffer _SamplePixelMapping
{
    uint samplePixelMapping[];
};

layout(BINDING(0, 5)) uniform accelerationStructureEXT Scene;

RisResult ris(inout uint rngState,
    vec3 baseColor, mat3 worldToTangent, mat3 tangentToWorld,
    vec3 normal, bool frontFace,
    vec3 x1)
{
#if 0
    LightSample lightSample = _sampleUniformLight(randomUniformFloat4(rngState), x1);

    ReservoirData reservoirData = ReservoirData(lightSample.triangle,
        lightSample.blasInstance, lightSample.uv, 1.0);
    Reservoir reservoir = Reservoir_default();
    reservoir.contributionWeight = 1.0 / lightSample.pdf;
    reservoir.sampleCount = 1.0;
    return RisResult(reservoirData, reservoir);
#else

    const uint M_AREA = 32;

    ReservoirData reservoirData = ReservoirData(0, 0, vec2(0.0), 0.0);
    Reservoir reservoir = Reservoir_default();
	
    vec3 outputSampleDirection;
    float outputSampleHitT;

    #pragma unroll
	for (uint i = 0; i < M_AREA; i++)
	{
        LightSample lightSample = _sampleUniformLight(randomUniformFloat4(rngState), x1);
        vec3 wInWorldSpace = lightSample.sampleDirection;
    
        float p_hat = 0.0;
        if (dot(wInWorldSpace, normal) > 0.0)
        {
            vec3 brdfEval = diffuseBsdfEval(baseColor);
            vec3 brdfContribution = bsdfContribution(brdfEval, normal, wInWorldSpace, 1.0);
            vec3 intensity = lightIntensity(lightSample.triangle, lightSample.blasInstance, lightSample.sampleDirection, lightSample.hitT, lightSample.transformedTriangle);

            p_hat = length(brdfContribution * intensity);
        }

        float weight = p_hat / lightSample.pdf;
        if (Reservoir_update(reservoir, weight, 1.0, rngState))
        {
            reservoirData = ReservoirData(lightSample.triangle,
                lightSample.blasInstance, lightSample.uv, p_hat);
            
            outputSampleDirection = lightSample.sampleDirection;
            outputSampleHitT = lightSample.hitT;
        }
	}

    
    reservoir.contributionWeight = (1.0 / max(reservoirData.p_hat, 0.00001)) * (reservoir.weightSum / max(reservoir.sampleCount, 0.00001));
    reservoir.contributionWeight = fixNan(reservoir.contributionWeight);

    if (ReservoirData_isValid(reservoirData))
    {
        float visibility = traceValidationRay(Scene, x1, outputSampleDirection, normal, outputSampleHitT) ? 0.0 : 1.0;
        reservoir.contributionWeight *= visibility;
    }

    return RisResult(reservoirData, reservoir);
#endif
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);
    uint id = pixel.y * constants.resolution.x + pixel.x;
    uint globalId = globalPixel.y * constants.globalResolution.x + globalPixel.x;

    //if (id >= sampleCount) return;
    //uint pid = id;//samplePixelMapping[id];

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));

    Intersection intersection = intersections[globalId];
    Ray ray = unpackRay(rays[globalId]);

    {
        vec3 intersectionPos = ray.origin + ray.direction * intersection.t;

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
        mat4 invTransTransform = blasInstance.invTransTransform;
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

        SampledMaterial material = sampleMaterial(materialDescriptors[blasInstance.materialIdx], texCoord);

        RisResult risResult = ris(rngState,
            material.baseColorFactor, worldToTangent, tangentToWorld,
            normal, intersection.frontFace,
            intersectionPos);

        outRestirReservoirs[id] = Reservoir_toPacked(risResult.reservoir);
        outRestirReservoirData[id] = ReservoirData_toPacked(risResult.reservoirData);
    }
}