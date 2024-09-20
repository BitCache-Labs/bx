#include "[engine]/shaders/Language.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/restir/restir.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"

const uint NUM_SPATIAL_SAMPLES = 5;

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 invView;
    mat4 invProj;
    uvec2 resolution;
    uint seed;
    uint spatialIndex;
    bool unbiased;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D gbuffer;

vec4 getPixelNormalAndDepth(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
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

bool validatePixelSimilarity(vec4 normalAndDepth, vec4 otherNormalAndDepth)
{
    return true;
    bool similairNormals = (1.0 - abs(dot(normalAndDepth.xyz, otherNormalAndDepth.xyz))) < 0.2;
    bool similairDepth = abs(1.0 - (normalAndDepth.w / otherNormalAndDepth.w)) < 0.1;
    return similairNormals && similairDepth;
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.resolution.x * constants.resolution.y) return;
    ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));
    
    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));

    Reservoir sampledReservoir = Reservoir_reconstructBiased(restirReservoirs[id]);
    ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);

    vec4 centerNormalAndDepth = getPixelNormalAndDepth(pixel);
    //restirReservoirs[id] = packRgb9e5(centerNormalAndDepth.xyz * 0.5 + 0.5).data; return;
    if (centerNormalAndDepth.w == 0.0)
    {
        outRestirReservoirs[id] = Reservoir_toPacked(sampledReservoir);
        outRestirReservoirData[id] = ReservoirData_toPacked(reservoirData);
        restirReservoirsHistory[id] = Reservoir_toPacked(sampledReservoir);
        restirReservoirDataHistory[id] = ReservoirData_toPacked(reservoirData);
        return;
    }
    vec3 centerOrigin = getPositionWs(pixel, centerNormalAndDepth.w);

    Reservoir reservoir = Reservoir_default();
    ReservoirStreamData streamData = ReservoirStreamData_default();

    Reservoir_mergeReservoirWithStream(reservoir, streamData, sampledReservoir,
        1.0, 1.0, 1.0, rngState);

    float screenRadius = constants.resolution.x / 30.0;
    float radius = screenRadius;
    float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 3 + constants.spatialIndex) * 0.5;
    ivec2 pixelSeed = (constants.spatialIndex == 0) ? (pixel >> 3) : (pixel >> 2);
    uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed * 3 + constants.spatialIndex));
    float samplingAngleOffset = angleSeed * (1.0 / float(0xffffffffU)) * TWO_PI;

    #pragma unroll
    for (uint i = 0; i < NUM_SPATIAL_SAMPLES; i++)
    {
        float angle = float(i) * GOLDEN_ANGLE + samplingAngleOffset;
        float currentRadius = pow(float(i) / NUM_SPATIAL_SAMPLES, 0.5) * radius + samplingRadiusOffset;

        ivec2 offset = ivec2(currentRadius * vec2(cos(angle), sin(angle)));
        uint flatOffset = offset.y * constants.resolution.x + offset.x;
        ivec2 samplePixel = pixel + offset;
        uint sampleId = id + flatOffset;

        // TODO: mirror
        if (samplePixel.x >= constants.resolution.x || samplePixel.y >= constants.resolution.y)
        {
            continue;
        }

        float visibility = 1.0;
        float jacobian = 1.0;
        float samplingWeight = 1.0;

        ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirData[sampleId]);

        vec4 sampleNormalAndDepth = getPixelNormalAndDepth(samplePixel);
        if (sampleNormalAndDepth.w == 0.0)
        {
            continue;
        }
        
        vec3 samplePositionWs = getPositionWs(samplePixel, sampleNormalAndDepth.w);
    
        vec3 sampleRelativeHitPos = sampledReservoirData.sampleDirection * sampledReservoirData.hitT;
        vec3 sampleRayHitWs = samplePositionWs + sampleRelativeHitPos;
        vec3 centerToSampleRelativePos = sampleRayHitWs - centerOrigin;
        vec3 centerOriginToSampleHit = normalize(centerToSampleRelativePos);

        // Trace if unbiased

        sampledReservoir = Reservoir_reconstructBiasedClamped(restirReservoirs[sampleId], 12.0);
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
    Reservoir_clampContributionWeight(reservoir, 10.0);

    outRestirReservoirs[id] = Reservoir_toPacked(reservoir);
    outRestirReservoirData[id] = ReservoirData_toPacked(reservoirData);

    restirReservoirsHistory[id] = Reservoir_toPacked(reservoir);
    restirReservoirDataHistory[id] = ReservoirData_toPacked(reservoirData);

    //
    //Reservoir reservoir = restirReservoirs[id];
    //RestirSample originalSample = reservoir.outputSample;
    //vec4 originalNormalAndDepth = getPixelNormalAndDepth(pixel);
    //
    //if (isReservoirValid(reservoir))
    //{
    //    float screenRadius = constants.width / 30.0;
    //    float radius = screenRadius; // TODO: use validity
    //
    //    float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 2 + constants.spatialIndex) * 0.5;
    //
    //    ivec2 pixelSeed = (constants.spatialIndex == 0) ? (pixel >> 3) : (pixel >> 2);
    //    uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed * 2 + constants.spatialIndex));
    //    float samplingAngleOffset = angleSeed * (1.0 / float(0xffffffffU)) * TWO_PI;
    //
    //    #pragma unroll
    //    for (uint i = 0; i < NUM_SPATIAL_SAMPLES; i++)
    //    {
    //        float angle = float(i) * GOLDEN_ANGLE + samplingAngleOffset;
    //        float currentRadius = pow(float(i) / NUM_SPATIAL_SAMPLES, 0.5) * radius + samplingRadiusOffset;
    //
    //        ivec2 offset = ivec2(currentRadius * vec2(cos(angle), sin(angle)));
    //        
    //        ivec2 candidatePixel = pixel + offset;
    //        uint flatOffset = offset.y * constants.width + offset.x;
    //        if (candidatePixel.x >= constants.width || flatOffset >= constants.dispatchSize)
    //        {
    //            continue;
    //        }
    //        
    //        Reservoir candidateReservoir = restirReservoirs[id + flatOffset];
    //        vec4 otherNormalAndDepth = getPixelNormalAndDepth(pixel + offset);
    //    
    //        if (isReservoirValid(candidateReservoir) && validatePixelSimilarity(originalNormalAndDepth, otherNormalAndDepth))
    //        {
    //            reservoir = combineReservoirs(rngState, reservoir, candidateReservoir);
    //            reservoir.sampleCount = min(reservoir.sampleCount, 1024 * 64);
    //        }
    //    }
    //    
    //    reservoir.outputSample.x0 = originalSample.x0;
    //    reservoir.outputSample.x1 = originalSample.x1;
    //}
    //
    //outRestirReservoirs[id] = reservoir;
    //restirReservoirsHistory[id] = reservoir;
}