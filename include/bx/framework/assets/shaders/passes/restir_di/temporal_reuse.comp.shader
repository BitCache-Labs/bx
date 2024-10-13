#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS
#define MATERIAL_BINDINGS

#include "[engine]/shaders/ray_tracing/material.shader"
#include "[engine]/shaders/passes/restir_di/restir.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/ray_tracing/sky.shader"

#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/ray_tracing/light_picking.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 invView;
    mat4 invProj;
    mat4 prevInvView;
    mat4 prevInvProj;
    uvec2 globalResolution;
    uvec2 resolution;
    bool unbiased;
    uint seed;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout(BINDING(0, 1)) uniform accelerationStructureEXT Scene;

layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D gbufferHistory;
layout (BINDING(0, 4), rg16f) uniform image2D velocity;

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

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.resolution.x * constants.resolution.y) return;
    ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed + 1));

    vec2 velocity = imageLoad(velocity, globalPixel).xy;
    ivec2 prevPixel = pixel - ivec2(vec2(constants.resolution) * velocity);
    prevPixel = clamp(prevPixel, ivec2(0), ivec2(constants.resolution) - 1);
    uint prevId = prevPixel.y * constants.resolution.x + prevPixel.x;
    ivec2 prevGlobalPixel = globalPixel - ivec2(vec2(constants.globalResolution) * velocity);
    prevGlobalPixel = clamp(prevGlobalPixel, ivec2(0), ivec2(constants.globalResolution) - 1);
    uint prevGlobalId = prevGlobalPixel.y * constants.globalResolution.x + prevGlobalPixel.x;
    
    {
        Reservoir outputReservoir = Reservoir_default();
        ReservoirData outputReservoirData = ReservoirData_default();

        vec4 centerNormalAndDepth = getPixelNormalAndDepth(globalPixel);
        vec4 sampleNormalAndDepthHistory = getPixelNormalAndDepthHistory(prevGlobalPixel);
        vec3 origin = getPositionWs(globalPixel, centerNormalAndDepth.w); // TODO: fix for mirrors
        vec3 normal = centerNormalAndDepth.xyz;

        ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
        Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[id]);

        { // Current
            bool currentValid = centerNormalAndDepth.w != 0.0 && ReservoirData_isValid(reservoirData);
            if (!currentValid)
            {
                reservoirData.p_hat = 0.0;
            }
            
            if (Reservoir_update(outputReservoir,
                reservoirData.p_hat * reservoir.contributionWeight * reservoir.sampleCount,
                reservoir.sampleCount, rngState))
            {
                outputReservoirData = reservoirData;
            }
        }

        { // History
            ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirDataHistory[prevId]);
            Reservoir sampledReservoir = Reservoir_fromPacked(restirReservoirsHistory[prevId]);

            float MAX_SAMPLE_COUNT = 32.0;
            if (sampledReservoir.sampleCount > MAX_SAMPLE_COUNT * reservoir.sampleCount)
            {
                sampledReservoir.weightSum *= MAX_SAMPLE_COUNT * reservoir.sampleCount / sampledReservoir.sampleCount;
                sampledReservoir.sampleCount = MAX_SAMPLE_COUNT * reservoir.sampleCount;
            }

            bool historyValid = sampleNormalAndDepthHistory.w != 0.0 && ReservoirData_isValid(sampledReservoirData);
            historyValid = historyValid && dot(normal, sampleNormalAndDepthHistory.xyz) >= 0.5;

            sampledReservoirData.p_hat = 0.0;

            if (historyValid)
            {
                vec3 direction;
                float tMax;
                if (sampledReservoirData.triangleLightSource != U32_MAX)
                {
                    mat4 lightTransform = blasInstances[sampledReservoirData.blasInstance].transform;
                    LightSample reconstructedLightSample = sampleTriangleLight(sampledReservoirData.triangleLightSource, sampledReservoirData.hitUv, lightTransform, origin, 0.0);
                    direction = reconstructedLightSample.sampleDirection;
                    tMax = reconstructedLightSample.hitT;
                }
                else
                {
                    direction = sampleSunDirection(sampledReservoirData.hitUv);
                    tMax = SUN_DISTANCE;
                }
                
                if (dot(direction, normal) > 0.0)
                {
                    vec3 brdfEval = diffuseBsdfEval(vec3(0.7, 0.7, 0.7)); // TODO: pass basecolor around?
                    vec3 brdfContribution = bsdfContribution(brdfEval, normal, direction, 1.0);
                    vec3 intensity = lightIntensity(sampledReservoirData.triangleLightSource, sampledReservoirData.blasInstance,
                        direction, tMax);
                
                    float visibility = 1.0;
                    if (constants.unbiased)
                    {
                        visibility = traceValidationRay(Scene, origin, direction, normal, tMax) ? 0.0 : 1.0;
                    }

                    sampledReservoirData.p_hat = length(visibility * brdfContribution * intensity);
                }
            }

            if (Reservoir_update(outputReservoir,
                sampledReservoirData.p_hat * sampledReservoir.contributionWeight * sampledReservoir.sampleCount,
                sampledReservoir.sampleCount, rngState))
            {
                outputReservoirData = sampledReservoirData;
            }
        }
        
        if (outputReservoirData.p_hat > 0.0 && outputReservoir.sampleCount > 0.0)
            outputReservoir.contributionWeight = (1.0 / outputReservoirData.p_hat) * outputReservoir.weightSum / outputReservoir.sampleCount;
        else
            outputReservoir.contributionWeight = 0.0;

        Reservoir_clampContributionWeight(outputReservoir, RESERVOIR_CONTRIBUTION_CLAMP);

        restirReservoirs[id] = Reservoir_toPacked(outputReservoir);
        restirReservoirData[id] = ReservoirData_toPacked(outputReservoirData);
    }
}