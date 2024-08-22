#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/wfpt/payload.shader"

layout (BINDING(0, 0), std430) readonly buffer _ShadowRays
{
    Ray shadowRays[];
};
layout (BINDING(0, 1), std430) readonly buffer _ShadowRayDistances
{
    float shadowRayDistances[];
};
layout(BINDING(0, 2)) readonly buffer _ShadowRayCount
{
    uint shadowRayCount;
};

layout (BINDING(0, 3), std430) buffer _Payloads
{
    Payload payloads[];
};

layout (BINDING(0, 4), std430) readonly buffer _ShadowPixelMapping
{
    uint shadowPixelMapping[];
};

layout(BINDING(0, 5)) uniform accelerationStructureEXT Scene;

bool shadowRayHit(Ray ray, float tMax)
{
    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, ray.origin, RT_EPSILON, ray.direction, tMax);
	rayQueryProceedEXT(rayQuery);
    return rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= shadowRayCount) return;
    uint pid = shadowPixelMapping[id];

    Ray ray = shadowRays[id];
    float rayDistance = shadowRayDistances[id];

    if (!shadowRayHit(ray, rayDistance))
    {
        Payload payload = payloads[pid];
        vec3 throughput = payload.throughput;
        vec3 accumulated = payload.accumulated;

        vec3 emission = vec3(0.6, 0.6, 0.5); // TODO: sun sampling

        vec3 lightingContribution = throughput * emission;// pdf

        accumulated += lightingContribution;

        payload.throughput = throughput;
        payload.accumulated = accumulated;
        payloads[pid] = payload;
    }
}