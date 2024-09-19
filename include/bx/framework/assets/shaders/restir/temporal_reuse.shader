#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/restir/restir.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 prevClipToWorld;
    uint dispatchSize;
    uint seed;
    uvec2 resolution;
} constants;

layout(BINDING(0, 1)) uniform accelerationStructureEXT Scene;

layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D gbufferHistory;

vec4 getPixelNormalAndDepth(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    return vec4(normal, 1.0 / gbufferData.r);
}

vec4 getPixelNormalAndDepthHistory(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbufferHistory, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    return vec4(normal, 1.0 / gbufferData.r);
}

vec3 getPositionWs(ivec2 pixel, float depth)
{
    vec2 sampleUv = (vec2(pixel) + 0.5) / vec2(constants.resolution);
    vec4 sampleClipPos = vec4(uvToClip(sampleUv), depth, 1.0);
    vec4 samplePositionWsPreDiv = sampleClipPos * constants.prevClipToWorld;
    return samplePositionWsPreDiv.xyz / samplePositionWsPreDiv.w;
}

bool traceRay(vec3 origin, vec3 direction, float tMax)
{
    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin, RT_EPSILON, direction, tMax);
	rayQueryProceedEXT(rayQuery);
    return rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

//void applyVisibilityCheck(inout Reservoir reservoir)
//{
//    RestirSample currentSample = reservoir.outputSample;
//    vec3 visibilityDir = normalize(currentSample.x2 - currentSample.x1);
//    float visibilityLength = distance(currentSample.x2, currentSample.x1);
//    if (traceRay(currentSample.x1, visibilityDir, visibilityLength))
//    {
//        reservoir.weight = 0.0;
//    }
//}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.dispatchSize) return;
    ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed + 1));
    
    vec3 centerOrigin = getPositionWs(pixel, getPixelNormalAndDepthHistory(pixel).w);

    // early out if center depth is sky

    ReservoirData outputReservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
    Reservoir reservoir = Reservoir_reconstructBiased(restirReservoirs[id]);
    ReservoirStreamData streamData = ReservoirStreamData(reservoir.contributionWeight, 1.0);

    //Reservoir_mergeReservoirWithStream(reservoir, streamData, reservoir,
    //        1.0, 1.0, 1.0, rngState);

    //reservoir.sampleCount = 1;

    for (uint i = 0; i < 0; i++)
    {
        ivec2 offset = ivec2(0, 0);
        uint flatOffset = offset.y * constants.resolution.x + offset.x;
        ivec2 samplePixel = pixel + offset;
        uint sampleId = id + flatOffset;

        float visibility = 1.0;
        float jacobian = 1.0;
        float samplingWeight = 1.0;
        
        ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirDataHistory[sampleId]);

        //vec4 sampleNormalAndDepth = getPixelNormalAndDepth(id);
        vec4 sampleNormalAndDepthHistory = getPixelNormalAndDepthHistory(samplePixel);
        // skip if sky

        vec3 samplePositionWs = getPositionWs(samplePixel, sampleNormalAndDepthHistory.w);

        vec3 sampleRelativeHitPos = sampledReservoirData.sampleDirection * sampledReservoirData.hitT;
        vec3 sampleRayHitWs = samplePositionWs + sampleRelativeHitPos;
        vec3 centerToSampleRelativePos = sampleRayHitWs - centerOrigin;
        vec3 centerOriginToSampleHit = normalize(centerToSampleRelativePos);

        // check validity similarity

        // trace visibility

        // calc jacobian
        //outputReservoirData = ReservoirData(vec3(0.0), 10000000.0);

        Reservoir sampledReservoir = Reservoir_reconstructBiasedClamped(restirReservoirsHistory[sampleId], 12.0);
        if (sampledReservoir.contributionWeight < 1e-3)
        {
            continue;
        }

        if (Reservoir_mergeReservoirWithStream(reservoir, streamData, sampledReservoir,
            visibility, jacobian, samplingWeight, rngState))
        {
            //outputReservoirData.sampleDirection = centerOriginToSampleHit;
            //outputReservoirData.hitT = length(centerToSampleRelativePos);

            //outputReservoirData = ReservoirData(vec3(0.0), 10000000.0);
            outputReservoirData = sampledReservoirData;
        }
    }

    Reservoir_finishStream(reservoir, streamData);
    Reservoir_clampContributionWeight(reservoir, 10.0);

    restirReservoirs[id] = Reservoir_toPacked(reservoir);
    restirReservoirData[id] = ReservoirData_toPacked(outputReservoirData);

    //uint id = uint(gl_GlobalInvocationID.x);
    //if (id >= constants.dispatchSize) return;
    //
    //uint rngState = pcgHash(id ^ xorShiftU32(constants.seed + 1));
    //
    //Reservoir currentReservoir = restirReservoirs[id];
    //RestirSample currentSample = currentReservoir.outputSample;
    //Reservoir previousReservoir = restirReservoirsHistory[id];
    
    //if (isReservoirValid(currentReservoir))
    //{
    //    applyVisibilityCheck(currentReservoir);
    //}
    //
    //// TODO: double check if these branches are required?
    //if (!isReservoirValid(currentReservoir) && isReservoirValid(previousReservoir))
    //{
    //    restirReservoirs[id] = previousReservoir;
    //}
    //else if (isReservoirValid(currentReservoir) && !isReservoirValid(previousReservoir))
    //{
    //    restirReservoirs[id] = currentReservoir;
    //}
    //else if (isReservoirValid(previousReservoir))
    //{
    //    previousReservoir.sampleCount = min(previousReservoir.sampleCount, currentReservoir.sampleCount * 15);
    //
    //    Reservoir outputReservoir = combineReservoirs(rngState, currentReservoir, previousReservoir);
    //    outputReservoir.outputSample.x0 = currentSample.x0;
    //    outputReservoir.outputSample.x1 = currentSample.x1;
    //    restirReservoirs[id] = outputReservoir;
    //}
}