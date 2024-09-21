#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS

#include "[engine]/shaders/restir/restir.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/ray_tracing/sky.shader"

#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/passes/wfpt/payload.shader"
#include "[engine]/shaders/ray_tracing/light_picking.shader"

layout (BINDING(0, 0), std430) readonly buffer _ShadowRayOrigins
{
    vec4 shadowRayOrigins[];
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

    Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[pid]);
    ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[pid]);
    vec3 origin = shadowRayOrigins[pid].xyz;
    vec3 direction = reservoirData.sampleDirection;
    float tMax = reservoirData.hitT;

    Payload payload = payloads[pid];
    vec3 hitNormal = unpackNormalizedXyz10(payload.hitNormal, 0);

    if (dot(direction, hitNormal) > 0.0)
    {
        if (!shadowRayHit(origin + direction * RT_EPSILON, direction, tMax))
        {
            vec3 throughput = unpackRgb9e5(payload.throughput);
            vec3 accumulated = unpackRgb9e5(payload.accumulated);
            
            bool sunSample = (reservoirData.triangleLightSource == U32_MAX);
            vec3 emission;
            if (sunSample)
            {
                emission = vec3(sunIntensity(direction.y));
            }
            else
            {
                emission = vec3(10.0);
            }

            // TODO: this throughputs geometry term isn't valid anymore
            vec3 lightingContribution = (throughput * emission) * reservoir.contributionWeight;

            accumulated += lightingContribution;
            
            payload.throughput = packRgb9e5(throughput);
            payload.accumulated = packRgb9e5(accumulated);
            payloads[pid] = payload;
        }
    }
}