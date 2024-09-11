#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/restir/restir.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint dispatchSize;
    uint seed;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout(BINDING(0, 1)) uniform accelerationStructureEXT Scene;

bool traceRay(vec3 origin, vec3 direction, float tMax)
{
    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin, RT_EPSILON, direction, tMax);
	rayQueryProceedEXT(rayQuery);
    return rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

void applyVisibilityCheck(inout Reservoir reservoir)
{
    RestirSample currentSample = reservoir.outputSample;
    vec3 visibilityDir = normalize(currentSample.x2 - currentSample.x1);
    float visibilityLength = distance(currentSample.x2, currentSample.x1);
    if (traceRay(currentSample.x1, visibilityDir, visibilityLength))
    {
        reservoir.weight = 0.0;
    }
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.dispatchSize) return;
    
    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed + 1));
    
    Reservoir currentReservoir = restirReservoirs[id];
    RestirSample currentSample = currentReservoir.outputSample;
    Reservoir previousReservoir = restirReservoirsHistory[id];
    
    if (isReservoirValid(currentReservoir))
    {
        applyVisibilityCheck(currentReservoir);
    }

    // TODO: double check if these branches are required?
    if (!isReservoirValid(currentReservoir) && isReservoirValid(previousReservoir))
    {
        restirReservoirs[id] = previousReservoir;
    }
    else if (isReservoirValid(currentReservoir) && !isReservoirValid(previousReservoir))
    {
        restirReservoirs[id] = currentReservoir;
    }
    else if (isReservoirValid(previousReservoir))
    {
        previousReservoir.sampleCount = min(previousReservoir.sampleCount, currentReservoir.sampleCount * 8);

        Reservoir outputReservoir = combineReservoirs(rngState, currentReservoir, previousReservoir);
        outputReservoir.outputSample.x0 = currentSample.x0;
        outputReservoir.outputSample.x1 = currentSample.x1;
        restirReservoirs[id] = outputReservoir;
    }
}