#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

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
layout(BINDING(0, 2), std430) writeonly buffer _RayCount
{
    uint rayCount;
};

layout (BINDING(0, 3), std430) writeonly buffer _Intersections
{
    Intersection intersections[];
};

layout (BINDING(0, 4), std430) writeonly buffer _PixelMapping
{
    uint pixelMapping[];
};

layout(BINDING(0, 5)) uniform accelerationStructureEXT Scene;

layout (BINDING(0, 6), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 7), rgba32f) uniform image2D neGbuffer;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= rayCount) return;

    Ray ray = unpackRay(rays[id]);

    // TODO: use gbuffer for first bounce

    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsOpaqueEXT, 0xFF, ray.origin, RT_EPSILON, ray.direction, 1000.0);
	rayQueryProceedEXT(rayQuery);

    vec3 finalHitPos;
    uint hitDepth = 0;

    Intersection intersection;
	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        intersection.uv = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
        intersection.primitiveIdx = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
        intersection.blasInstanceIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
        //intersection.blasInstanceIdx = rayQueryGetIntersectionInstanceIdEXT(rayQuery, true);
        intersection.t = rayQueryGetIntersectionTEXT(rayQuery, true);    
	    intersection.frontFace = rayQueryGetIntersectionFrontFaceEXT(rayQuery, true);

        finalHitPos = ray.origin + ray.direction * intersection.t;

        // TODO: check material on hit, if mirror or portal, keep going
        // when reaching final hit update multi-bounce gbuffer
    }
    else
    {
        intersection.t = T_MISS;
    }

    if (intersection.t != T_MISS)
    {
        uint rayIdx = atomicAdd(rayCount, 1u);
        pixelMapping[rayIdx] = id;

        ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));
        imageStore(neGbuffer, pixel, vec4(finalHitPos, uintBitsToFloat(hitDepth)));
    }

    intersections[id] = intersection;
}