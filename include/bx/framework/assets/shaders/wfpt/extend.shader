#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#include "[engine]/shaders/ray_tracing/ray.shader"

layout (BINDING(0, 0), std430) readonly buffer _Rays
{
    Ray rays[];
};
layout(BINDING(0, 1)) readonly buffer _RayCount
{
    uint rayCount;
};

layout (BINDING(0, 2), std430) writeonly buffer _Intersections
{
    Intersection intersections[];
};

layout (BINDING(0, 3), std430) readonly buffer _PixelMapping
{
    uint pixelMapping[];
};

layout(BINDING(0, 4)) uniform accelerationStructureEXT Scene;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= rayCount) return;
    uint pid = pixelMapping[id];

    Ray ray = rays[id];

    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsOpaqueEXT, 0xFF, ray.origin, RT_EPSILON, ray.direction, 1000.0);
	rayQueryProceedEXT(rayQuery);

    Intersection intersection;
	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        intersection.uv = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
        intersection.primitiveIdx = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
        intersection.blasInstanceIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);// TODO: rayQueryGetIntersectionInstanceIdEXT??
        intersection.t = rayQueryGetIntersectionTEXT(rayQuery, true);    
	    intersection.frontFace = rayQueryGetIntersectionFrontFaceEXT(rayQuery, true);
    }
    else
    {
        intersection.t = T_MISS;
    }

    intersections[pid] = intersection;
}