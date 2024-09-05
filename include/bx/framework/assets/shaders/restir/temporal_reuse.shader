#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/restir/restir.shader"

const float MIS_HISTORY_FACTOR = 1.0;

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

float currentBalanceHeuristic(float pdfCurrent, float pdfPrev)
{
    return (pdfCurrent) / (MIS_HISTORY_FACTOR * pdfPrev + pdfCurrent);
}

float prevBalanceHeuristic(float pdfCurrent, float pdfPrev)
{
    return (MIS_HISTORY_FACTOR * pdfPrev) / (MIS_HISTORY_FACTOR * pdfPrev + pdfCurrent);
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.dispatchSize) return;
    
    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed + 1));

    RestirSample currentSample = restirSamples[id];
    RestirSample previousSample = restirSamplesHistory[id];

    vec3 visibilityDir = normalize(currentSample.x2 - currentSample.x1);
    float visibilityLength = distance(currentSample.x2, currentSample.x1);
    if (traceRay(currentSample.x1, visibilityDir, visibilityLength))
    {
        currentSample.unoccludedContributionWeight = 0.0;
        restirSamples[id] = currentSample;
        return;
    }

    if (!isRestirSampleValid(currentSample))
    {
        //restirSamples[id] = previousSample;
        //return;
    }

    Reservoir reservoir = makeReservoir();

    float currentPdf = currentSample.unoccludedContributionWeight;
    float prevPdf = previousSample.unoccludedContributionWeight;
    
    float weight = currentBalanceHeuristic(currentPdf, prevPdf) * currentSample.unoccludedContributionWeight * currentSample.weight;
    updateReservoir(reservoir, rngState, currentSample, weight);
    
    //weight = prevBalanceHeuristic(currentPdf, prevPdf) * previousSample.unoccludedContributionWeight * previousSample.weight;
    //updateReservoir(reservoir, rngState, previousSample, weight);
    
    RestirSample outputSample = reservoir.outputSample;
    outputSample.x0 = currentSample.x0;
    outputSample.x1 = currentSample.x1;
    outputSample.weight = (1.0 / outputSample.unoccludedContributionWeight) * reservoir.weightSum;
    
    restirSamples[id] = outputSample;
}