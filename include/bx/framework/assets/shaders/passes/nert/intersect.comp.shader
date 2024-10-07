#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define BLAS_DATA_BINDINGS
#define MATERIAL_BINDINGS

#include "[engine]/shaders/ray_tracing/material.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"

#include "[engine]/shaders/ray_tracing/ray.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint maxBounces;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), std430) readonly buffer _Rays
{
    PackedRay rays[];
};
layout(BINDING(0, 2), std430) writeonly buffer _SampleCount
{
    uint sampleCount;
};

layout (BINDING(0, 3), std430) writeonly buffer _Intersections
{
    Intersection intersections[];
};

layout (BINDING(0, 4), std430) writeonly buffer _SamplePixelMapping
{
    uint samplePixelMapping[];
};

layout(BINDING(0, 6)) uniform accelerationStructureEXT Scene;

layout (BINDING(0, 7), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 8), rgba32f) uniform image2D neGbuffer;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.resolution.x * constants.resolution.y) return;

    Ray ray = unpackRay(rays[id]);

    // TODO: use gbuffer for first bounce

    vec3 finalHitPos;
    Intersection intersection;

    uint hitDepth = 0;
    for (; hitDepth < constants.maxBounces; hitDepth++)
    {
        rayQueryEXT rayQuery;
	    rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsOpaqueEXT, 0xFF, ray.origin, RT_EPSILON, ray.direction, 1000.0);
	    rayQueryProceedEXT(rayQuery);

        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
        {
            intersection.uv = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
            intersection.primitiveIdx = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
            intersection.blasInstanceIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
            //intersection.blasInstanceIdx = rayQueryGetIntersectionInstanceIdEXT(rayQuery, true);
            intersection.t = rayQueryGetIntersectionTEXT(rayQuery, true);    
	        intersection.frontFace = rayQueryGetIntersectionFrontFaceEXT(rayQuery, true);

            finalHitPos = ray.origin + ray.direction * intersection.t;

            BlasInstance blasInstance = blasInstances[intersection.blasInstanceIdx];
            if (materialDescriptors[blasInstance.materialIdx].isMirror)
            {
                BlasAccessor blasAccessor = blasAccessors[blasInstance.blasIdx];

                Triangle triangle = blasTriangles[intersection.primitiveIdx + blasAccessor.triangleOffset];
                PackedVertex vertex0 = blasVertices[triangle.i0 + blasAccessor.vertexOffset];
                PackedVertex vertex1 = blasVertices[triangle.i1 + blasAccessor.vertexOffset];
                PackedVertex vertex2 = blasVertices[triangle.i2 + blasAccessor.vertexOffset];

                vec3 normal0 = unpackNormalizedXyz10(vertex0.normal, 0);
                vec3 normal1 = unpackNormalizedXyz10(vertex1.normal, 0);
                vec3 normal2 = unpackNormalizedXyz10(vertex2.normal, 0);

                vec3 barycentrics = barycentricsFromUv(intersection.uv);
                vec3 normal = normalize(normal0 * barycentrics.x
                    + normal1 * barycentrics.y
                    + normal2 * barycentrics.z);

                // Correct normal for transform and backface hits
                mat4 invTransTransform = blasInstance.invTransTransform;
                normal = normalize((invTransTransform * vec4(normal, 1.0)).xyz);
                if (!intersection.frontFace)
                {
                    normal = -normal;
                }

                ray.direction = reflect(ray.direction, normal);
                ray.origin = finalHitPos;
            }
            else
            {
                break;
            }
            // TODO: check material on hit, if mirror or portal, keep going
            // when reaching final hit update multi-bounce gbuffer
        }
        else
        {
            intersection.t = T_MISS;
            break;
        }
    }

    if (intersection.t != T_MISS) // && hit diffuse on final hit!
    {
        uint sampleIdx = atomicAdd(sampleCount, 1u);
        samplePixelMapping[sampleIdx] = id;

        ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));
        imageStore(neGbuffer, pixel, vec4(finalHitPos, uintBitsToFloat(hitDepth)));
    }

    intersections[id] = intersection;
}