#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define MATERIAL_BINDINGS
#define RESTIR_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS

#include "[engine]/shaders/ray_tracing/material.shader"
#include "[engine]/shaders/restir/restir.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/ray_tracing/sky.shader"

#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/ray_tracing/light_picking.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 2), std430) readonly buffer _InverseSamplePixelMapping
{
    uint inverseSamplePixelMapping[];
};

layout (BINDING(0, 3), std430) readonly buffer _Intersections
{
    Intersection intersections[];
};

layout(BINDING(0, 4)) uniform accelerationStructureEXT Scene;

layout (BINDING(0, 5), rgba32f) uniform image2D neGbuffer;
layout (BINDING(0, 6), rgba32f) uniform image2D outImage;

bool shadowRayHit(vec3 origin, vec3 direction, float tMax)
{
    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin, RT_EPSILON, direction, tMax);
	rayQueryProceedEXT(rayQuery);
    return rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    uint sampleId = inverseSamplePixelMapping[id];

    Intersection intersection = intersections[id];

    Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[sampleId]);
    ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[sampleId]);
    vec3 origin = imageLoad(neGbuffer, pixel).rgb;
    vec3 direction = reservoirData.sampleDirection;
    float tMax = reservoirData.hitT;

    // TODO: write in intersect.comp for mirrors, load here
    vec3 throughput = vec3(1.0);

    vec3 lightingContribution = vec3(0.0);

    //Payload payload = payloads[pid];
    //vec3 hitNormal = unpackNormalizedXyz10(payload.hitNormal, 0);
    //
    //if (dot(direction, hitNormal) > 0.0)
    if (intersection.t != T_MISS)
    {
        if (!shadowRayHit(origin + direction * RT_EPSILON, direction, tMax))
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
            mat4 invTransTransform = transpose(blasInstance.invTransform);
            normal = normalize((invTransTransform * vec4(normal, 1.0)).xyz);
            if (!intersection.frontFace)
            {
                normal = -normal;
            }

            // Build tangent to world matrix and correct for wOut below hemisphere (can happen due to normal mapping)
            //mat3 tangentToWorld = buildOrthonormalBasis(normal);
            //mat3 worldToTangent = transpose(tangentToWorld);
            //
            //vec3 wOutTangentSpace = normalize(worldToTangent * -ray.direction);
            //if (wOutTangentSpace.z < 0.0 && intersection.frontFace)
            //{
            //    wOutTangentSpace.z *= -0.25;
            //    wOutTangentSpace = normalize(wOutTangentSpace);
            //}
            
            SampledMaterial material = sampleMaterial(materialDescriptors[blasInstance.materialIdx], texCoord);

            // TODO: load from material / evaluate sun
            vec3 emission = vec3(4.0);

            vec3 brdfEval = diffuseBsdfEval(material.baseColorFactor);
            vec3 brdfContribution = bsdfContribution(brdfEval, normal, direction, 1.0); // TODO: pdf value here???
            throughput *= brdfContribution;

            lightingContribution = (throughput * emission);// * reservoir.contributionWeight;
            //lightingContribution = vec3(1.0, 0.0, 1.0);
        }
    }

    imageStore(outImage, pixel, vec4(lightingContribution, 1.0));
}