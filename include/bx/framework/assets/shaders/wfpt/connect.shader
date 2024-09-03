#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/wfpt/payload.shader"
#include "[engine]/shaders/restir/restir.shader"

//layout (BINDING(0, 0), std430) readonly buffer _ShadowRays
//{
//    PackedRay shadowRays[];
//};
//layout (BINDING(0, 1), std430) readonly buffer _ShadowRayDistances
//{
//    float shadowRayDistances[];
//};
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

bool shadowRayHit(vec3 origin, vec3 direction, float tMax)
{
    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin, RT_EPSILON, direction, tMax);
	rayQueryProceedEXT(rayQuery);
    return rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= shadowRayCount) return;
    uint pid = shadowPixelMapping[id];

    // Ray ray = unpackRay(shadowRays[id]);
    // float rayDistance = shadowRayDistances[id];

    RestirSample lightSample = restirSamples[pid];
    vec3 origin = lightSample.x1.xyz;
    vec3 direction = normalize(lightSample.x2.xyz - lightSample.x1.xyz);
    float tMax = distance(lightSample.x2.xyz, lightSample.x1.xyz);

    Payload payload = payloads[pid];
    vec3 hitNormal = unpackNormalizedXyz10(payload.hitNormal, 0);

    if (dot(direction, hitNormal) > 0.0)
    {
        if (!shadowRayHit(origin + direction * RT_EPSILON, direction, tMax))
        {
            vec3 throughput = unpackRgb9e5(payload.throughput);
            vec3 accumulated = unpackRgb9e5(payload.accumulated);
            
            vec3 emission = vec3(4.0);//vec3(0.6, 0.6, 0.5); // TODO: sun sampling
            
            vec3 lightingContribution = (throughput * emission) * lightSample.weight;
            
            accumulated += lightingContribution;
            
            payload.throughput = packRgb9e5(throughput);
            payload.accumulated = packRgb9e5(accumulated);
            payloads[pid] = payload;
        }
    }
}