#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define MATERIAL_BINDINGS
#define RESTIR_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS

#include "[engine]/shaders/ray_tracing/material.shader"
#include "[engine]/shaders/passes/restir_di/restir.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/ray_tracing/sky.shader"

#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/ray_tracing/light_picking.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 globalResolution;
    uvec2 resolution;
    uint sampleNumber;
    uint seed;
    bool ibl;
    uint _PADDING0;
} constants;

layout (BINDING(0, 3), std430) readonly buffer _Intersections
{
    Intersection intersections[];
};

layout(BINDING(0, 4)) uniform accelerationStructureEXT Scene;

layout (BINDING(0, 5), rgba32f) uniform image2D neGbuffer;
layout (BINDING(0, 6), rgba32f) uniform image2D outIllumination;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);
    uint globalId = uint(globalPixel.y * constants.globalResolution.x + globalPixel.x);

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));

    Intersection intersection = intersections[globalId];
    vec3 origin = imageLoad(neGbuffer, globalPixel).rgb;

    Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[id]);
    ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
    
    vec3 direction = UP;
    float tMax = 0.0;
    if (ReservoirData_isValid(reservoirData))
    {
        if (reservoirData.triangleLightSource != U32_MAX)
        {
            mat4 lightTransform = blasInstances[reservoirData.blasInstance].transform;
            LightSample reconstructedLightSample = sampleTriangleLight(reservoirData.triangleLightSource, reservoirData.hitUv, lightTransform, origin, 0.0);
            direction = reconstructedLightSample.sampleDirection;
            tMax = reconstructedLightSample.hitT;
        }
        else
        {
            direction = sampleSunDirection(reservoirData.hitUv);
            tMax = SUN_DISTANCE;
        }
    }

    vec3 lightingContribution = vec3(0.0);

    if (intersection.t != T_MISS)
    {
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
        
        SampledMaterial material = sampleMaterial(materialDescriptors[blasInstance.materialIdx], texCoord);
        
        if (ReservoirData_isValid(reservoirData))
        {
            vec3 brdfContribution = bsdfContribution(vec3(1.0), normal, direction, 1.0);
            vec3 intensity = lightIntensity(reservoirData.triangleLightSource, reservoirData.blasInstance, direction, tMax);

            float visibility = 0.0;
            if (dot(normal, direction) > 0.0)
            {
                visibility = traceValidationRay(Scene, origin, direction, normal, tMax) ? 0.0 : 1.0;
            }

            vec3 radiance = visibility * intensity * brdfContribution;
            lightingContribution += radiance * reservoir.contributionWeight;
        }

        if (constants.ibl)
        {
            mat3 tangentToWorld = buildOrthonormalBasis(normal);

            DiffuseLobe diffuseLobe = DiffuseLobe(material.baseColorFactor);
            BsdfSample bsdfSample = sampleDiffuseBsdf(diffuseLobe, randomUniformFloat2(rngState));
            BsdfEval bsdfEval = evalDiffuseBsdf(diffuseLobe);
            
            vec3 throughput = vec3(1.0);
            vec3 wInWorldSpace;
            
            if (applyBsdf(bsdfSample, bsdfEval, tangentToWorld, normal, throughput, wInWorldSpace))
            {
                float visibility = traceValidationRay(Scene, origin, wInWorldSpace, normal, 100.0) ? 0.0 : 1.0;
                lightingContribution += throughput * shadeSky(wInWorldSpace) * visibility * 100.0;
            }
        }
    }

    imageStore(outIllumination, pixel, vec4(lightingContribution, 1.0));

    //vec3 old = imageLoad(outImage, pixel).rgb;
    //float portion = 1.0 / (0 + 1); // constants.sampleNumber
    //vec3 resolved = (old * (1.0 - portion) + portion * lightingContribution);
    //imageStore(outImage, pixel, vec4(resolved, 1.0));
}