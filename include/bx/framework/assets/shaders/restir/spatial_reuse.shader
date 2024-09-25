#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS

#include "[engine]/shaders/restir/restir.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/ray_tracing/sky.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/ray_tracing/light_picking.shader"
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
    bool jacobian;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D gbuffer;

layout(BINDING(0, 2)) uniform accelerationStructureEXT Scene;

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

bool traceValidationRay(vec3 origin, vec3 direction, float tMax)
{
    const float validationEpsilon = min(tMax * 0.001, 0.1);
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
    
    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));
    
    ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
    Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[id]);
    
    vec4 centerNormalAndDepth = getPixelNormalAndDepth(pixel);
    if (centerNormalAndDepth.w == 0.0)
    {
        outRestirReservoirs[id] = Reservoir_toPacked(reservoir);
        outRestirReservoirData[id] = ReservoirData_toPacked(reservoirData);
        restirReservoirsHistory[id] = Reservoir_toPacked(reservoir);
        restirReservoirDataHistory[id] = ReservoirData_toPacked(reservoirData);
        return;
    }
    vec3 origin = getPositionWs(pixel, centerNormalAndDepth.w);
    vec3 normal = centerNormalAndDepth.xyz;
    
    Reservoir outputReservoir = Reservoir_default();

    Reservoir_update(outputReservoir,
            reservoirData.p_hat * reservoir.contributionWeight * reservoir.sampleCount,
            reservoir.sampleCount, rngState);

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
        samplePixel = mirrorSample(samplePixel, ivec2(constants.resolution));
    
        ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirData[sampleId]);
    
        vec4 sampleNormalAndDepth = getPixelNormalAndDepth(samplePixel);
        if (sampleNormalAndDepth.w == 0.0)
        {
            continue;
        }
    
        Reservoir sampledReservoir = Reservoir_fromPackedClamped(restirReservoirs[sampleId], RESERVOIR_M_CLAMP);
    
        mat4 lightTransform = inverse(blasInstances[sampledReservoirData.blasInstance].invTransform);
        LightSample sampledLightSample = sampleTriangleLight(sampledReservoirData.triangleLightSource, sampledReservoirData.hitUv, lightTransform, origin, 0.0);
        
        if (dot(sampledLightSample.sampleDirection, normal) > 0.0)
        {
            vec3 brdfEval = diffuseBsdfEval(vec3(0.7, 0.7, 0.7)); // TODO: pass basecolor around?
            vec3 brdfContribution = bsdfContribution(brdfEval, normal, sampledLightSample.sampleDirection, 1.0);
            float intensity = triangleLightIntensity(sampledReservoirData.triangleLightSource, sampledReservoirData.blasInstance,
                sampledLightSample.sampleDirection, sampledLightSample.hitT);
        
            float visibility = 1.0;
            if (constants.unbiased)
            {
                visibility = traceValidationRay(origin, sampledLightSample.sampleDirection, sampledLightSample.hitT) ? 0.0 : 1.0;
            }

            sampledReservoirData.p_hat = length(visibility * brdfContribution * intensity);
        }
        else
        {
            sampledReservoirData.p_hat = 0.0;
        }

        if (Reservoir_update(outputReservoir,
            sampledReservoirData.p_hat * sampledReservoir.contributionWeight * sampledReservoir.sampleCount,
            sampledReservoir.sampleCount, rngState))
        {
            reservoirData = sampledReservoirData;
        }
    }

    outputReservoir.contributionWeight = (reservoirData.p_hat > 0.0) ? ((1.0 / reservoirData.p_hat) * outputReservoir.weightSum / outputReservoir.sampleCount) : 0.0;
    
    Reservoir_clampContributionWeight(outputReservoir, RESERVOIR_CONTRIBUTION_CLAMP);

    outRestirReservoirs[id] = Reservoir_toPacked(outputReservoir);
    outRestirReservoirData[id] = ReservoirData_toPacked(reservoirData);
    
    restirReservoirsHistory[id] = Reservoir_toPacked(outputReservoir);
    restirReservoirDataHistory[id] = ReservoirData_toPacked(reservoirData);

    //uint id = uint(gl_GlobalInvocationID.x);
    //if (id >= constants.resolution.x * constants.resolution.y) return;
    //ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));
    //
    //uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));
    //
    //Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[id]);
    //ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
    //
    //vec4 centerNormalAndDepth = getPixelNormalAndDepth(pixel);
    //if (centerNormalAndDepth.w == 0.0)
    //{
    //    outRestirReservoirs[id] = Reservoir_toPacked(reservoir);
    //    outRestirReservoirData[id] = ReservoirData_toPacked(reservoirData);
    //    restirReservoirsHistory[id] = Reservoir_toPacked(reservoir);
    //    restirReservoirDataHistory[id] = ReservoirData_toPacked(reservoirData);
    //    return;
    //}
    //vec3 centerOrigin = getPositionWs(pixel, centerNormalAndDepth.w);
    //
    //float screenRadius = constants.resolution.x / 30.0;
    //float radius = screenRadius;
    //float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 3 + constants.spatialIndex) * 0.5;
    //ivec2 pixelSeed = (constants.spatialIndex == 0) ? (pixel >> 3) : (pixel >> 2);
    //uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed * 3 + constants.spatialIndex));
    //float samplingAngleOffset = angleSeed * (1.0 / float(0xffffffffU)) * TWO_PI;
    //
    //#pragma unroll
    //for (uint i = 0; i < NUM_SPATIAL_SAMPLES; i++)
    //{
    //    float angle = float(i) * GOLDEN_ANGLE + samplingAngleOffset;
    //    float currentRadius = pow(float(i) / NUM_SPATIAL_SAMPLES, 0.5) * radius + samplingRadiusOffset;
    //
    //    ivec2 offset = ivec2(currentRadius * vec2(cos(angle), sin(angle)));
    //    uint flatOffset = offset.y * constants.resolution.x + offset.x;
    //    ivec2 samplePixel = pixel + offset;
    //    uint sampleId = id + flatOffset;
    //
    //    samplePixel = mirrorSample(samplePixel, ivec2(constants.resolution));
    //
    //    ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirData[sampleId]);
    //
    //    vec4 sampleNormalAndDepth = getPixelNormalAndDepth(samplePixel);
    //    if (sampleNormalAndDepth.w == 0.0)
    //    {
    //        continue;
    //    }
    //    
    //    vec3 samplePositionWs = getPositionWs(samplePixel, sampleNormalAndDepth.w);
    //
    //    vec3 sampleRelativeHitPos = sampledReservoirData.sampleDirection * sampledReservoirData.hitT;
    //    vec3 sampleRayHitWs = samplePositionWs + sampleRelativeHitPos;
    //    vec3 centerToSampleRelativePos = sampleRayHitWs - centerOrigin;
    //    vec3 centerOriginToSampleHit = normalize(centerToSampleRelativePos);
    //
    //    // Trace if unbiased
    //
    //    Reservoir sampledReservoir = Reservoir_fromPackedClamped(restirReservoirs[sampleId], RESERVOIR_M_CLAMP);
    //
    //    float jacobian = 1.0;
    //    if (constants.jacobian)
    //    {
    //        jacobian = ReservoirData_getLightWeight(sampledReservoirData, centerOrigin) / ReservoirData_getLightWeight(reservoirData, centerOrigin);
    //    }
    //    sampledReservoirData.unoccludedContributionWeight *= jacobian;
    //
    //    if (sampledReservoir.contributionWeight < 1e-3)
    //    {
    //        continue;
    //    }
    //
    //    if (constants.unbiased && traceValidationRay(centerOrigin, centerOriginToSampleHit, length(centerToSampleRelativePos)))
    //    {
    //        sampledReservoir.contributionWeight = 0.0;
    //    }
    //
    //    bool firstReservoirWasPicked;
    //    reservoir = Reservoir_combineReservoirs(reservoir, reservoirData.unoccludedContributionWeight, sampledReservoir, sampledReservoirData.unoccludedContributionWeight,
	//        rngState, firstReservoirWasPicked);
    //    if (!firstReservoirWasPicked)
    //    {
    //        reservoirData.sampleDirection = centerOriginToSampleHit;
    //        reservoirData.hitT = length(centerToSampleRelativePos);
    //        reservoirData.unoccludedContributionWeight = sampledReservoirData.unoccludedContributionWeight;
    //    }
    //}
    //
    //Reservoir_clampContributionWeight(reservoir, RESERVOIR_M_CLAMP);
    //
    //outRestirReservoirs[id] = Reservoir_toPacked(reservoir);
    //outRestirReservoirData[id] = ReservoirData_toPacked(reservoirData);
    //
    //restirReservoirsHistory[id] = Reservoir_toPacked(reservoir);
    //restirReservoirDataHistory[id] = ReservoirData_toPacked(reservoirData);
}