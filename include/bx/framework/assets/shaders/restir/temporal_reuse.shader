#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/restir/restir.shader"
#include "[engine]/shaders/sampling.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 invView;
    mat4 invProj;
    mat4 prevInvView;
    mat4 prevInvProj;
    uvec2 resolution;
    bool unbiased;
    uint seed;
} constants;

layout(BINDING(0, 1)) uniform accelerationStructureEXT Scene;

layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D gbufferHistory;

vec4 getPixelNormalAndDepth(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

vec4 getPixelNormalAndDepthHistory(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbufferHistory, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

vec3 getPositionWs(ivec2 pixel, float depth)
{
    vec2 pixelCenter = vec2(pixel.x + 0.5, pixel.y + 0.5);
    vec2 uv = (pixelCenter / vec2(constants.resolution)) * 2.0 - 1.0;
    uv.y = -uv.y;
    vec4 origin = constants.invView * vec4(0.0, 0.0, 0.0, 1.0);
    vec4 target = constants.invProj * vec4(uv, 1.0, 1.0);
    vec4 direction = constants.invView * vec4(normalize(target.xyz), 0.0);
    return origin.xyz + direction.xyz * depth;
}

vec3 getPositionWsHistory(ivec2 pixel, float depth)
{
    vec2 pixelCenter = vec2(pixel.x + 0.5, pixel.y + 0.5);
    vec2 uv = (pixelCenter / vec2(constants.resolution)) * 2.0 - 1.0;
    uv.y = -uv.y;
    vec4 origin = constants.prevInvView * vec4(0.0, 0.0, 0.0, 1.0);
    vec4 target = constants.prevInvProj * vec4(uv, 1.0, 1.0);
    vec4 direction = constants.prevInvView * vec4(normalize(target.xyz), 0.0);
    return origin.xyz + direction.xyz * depth;
}

bool traceValidationRay(vec3 origin, vec3 direction, float tMax)
{
    const float validationEpsilon = tMax * 0.1;
    origin += validationEpsilon * direction;
    tMax = max(0.0, tMax - validationEpsilon);

    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin, RT_EPSILON, direction, tMax);
	rayQueryProceedEXT(rayQuery);
    return rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.resolution.x * constants.resolution.y) return;
    ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed + 1));
    
    vec4 centerNormalAndDepth = getPixelNormalAndDepth(pixel);

    //restirReservoirs[id] = packRgb9e5(centerNormalAndDepth.xyz * 0.5 + 0.5).data; return;

    if (centerNormalAndDepth.w == 0.0)
    {
        return;
    }
    vec3 centerOrigin = getPositionWs(pixel, centerNormalAndDepth.w);

    ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
    Reservoir reservoir = Reservoir_default();
    ReservoirStreamData streamData = ReservoirStreamData_default();
    streamData.contributionWeightFactor = reservoirData.contributionWeightFactor;

    if (traceValidationRay(centerOrigin, reservoirData.sampleDirection, reservoirData.hitT))
    {
        reservoir.contributionWeight = 0.0;
    }

    Reservoir sampledReservoir = Reservoir_reconstructBiased(restirReservoirs[id]);
    Reservoir_mergeReservoirWithStream(reservoir, streamData, sampledReservoir,
        1.0, 1.0, 1.0, rngState);
    
    float screenRadius = constants.resolution.x / 30.0;
    float radius = screenRadius;
    float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 3 + 3) * 0.5;
    ivec2 pixelSeed = (pixel >> 3);
    uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed * 3 + 3));
    float samplingAngleOffset = angleSeed * (1.0 / float(0xffffffffU)) * TWO_PI;

    for (uint i = 0; i < 1; i++)
    {
        //float angle = float(i) * GOLDEN_ANGLE + samplingAngleOffset;
        //float currentRadius = pow(float(i) / NUM_TEMPORAL_SAMPLES, 0.5) * radius + samplingRadiusOffset;

        ivec2 offset = ivec2(0);//ivec2(currentRadius * vec2(cos(angle), sin(angle)));
        uint flatOffset = offset.y * constants.resolution.x + offset.x;
        ivec2 samplePixel = pixel + offset;
        uint sampleId = id + flatOffset;
    
        float visibility = 1.0;
        float jacobian = 1.0;
        float samplingWeight = 1.0;
        
        ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirDataHistory[sampleId]);
    
        vec4 sampleNormalAndDepthHistory = getPixelNormalAndDepthHistory(samplePixel);
        if (sampleNormalAndDepthHistory.w == 0.0)
        {
            continue;
        }
    
        vec3 samplePositionWs = getPositionWsHistory(samplePixel, sampleNormalAndDepthHistory.w);
    
        vec3 sampleRelativeHitPos = sampledReservoirData.sampleDirection * sampledReservoirData.hitT;
        vec3 sampleRayHitWs = samplePositionWs + sampleRelativeHitPos;
        vec3 centerToSampleRelativePos = sampleRayHitWs - centerOrigin;
        vec3 centerOriginToSampleHit = normalize(centerToSampleRelativePos);

        if (constants.unbiased)
        {
            if (traceValidationRay(centerOrigin, centerOriginToSampleHit, length(centerToSampleRelativePos)))
            {
                visibility = 0.0;
            }
        }
    
        sampledReservoir = Reservoir_reconstructBiasedClamped(restirReservoirsHistory[sampleId], 12.0);
        if (sampledReservoir.contributionWeight < 1e-3)
        {
            continue;
        }

        if (Reservoir_mergeReservoirWithStream(reservoir, streamData, sampledReservoir,
            visibility, 1.0, 1.0, rngState))
        {
            reservoirData.sampleDirection = centerOriginToSampleHit;
            reservoirData.hitT = length(centerToSampleRelativePos);
        }
    }

    Reservoir_finishStream(reservoir, streamData);
    reservoirData.contributionWeightFactor = streamData.contributionWeightFactor;

    Reservoir_clampContributionWeight(reservoir, 10.0);

    restirReservoirs[id] = Reservoir_toPacked(reservoir);
    restirReservoirData[id] = ReservoirData_toPacked(reservoirData);
}