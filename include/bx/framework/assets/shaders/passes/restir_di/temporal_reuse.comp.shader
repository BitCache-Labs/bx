#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#define RESTIR_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS

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
    uvec2 resolution;
    bool unbiased;
    uint seed;
} constants;

layout(BINDING(0, 1)) uniform accelerationStructureEXT Scene;

layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D gbufferHistory;
layout (BINDING(0, 4), rgba32f) uniform image2D velocity;

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

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed + 1));

    vec4 centerNormalAndDepth = getPixelNormalAndDepth(pixel);

    ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
    Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[id]);
    
    vec2 velocity = imageLoad(velocity, pixel).rg / 100.0;
    ivec2 prevPixel = ivec2(vec2(pixel) - (vec2(constants.resolution) * velocity));
    if (prevPixel.x >= constants.resolution.x || prevPixel.y >= constants.resolution.y || prevPixel.x < 0 || prevPixel.y < 0)
    {
        prevPixel = pixel;
    }
    uint prevId = prevPixel.y * constants.resolution.x + prevPixel.x;

    ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirDataHistory[prevId]);

    vec4 sampleNormalAndDepthHistory = getPixelNormalAndDepthHistory(prevPixel);

    if (sampleNormalAndDepthHistory.w != 0.0 && centerNormalAndDepth.w != 0.0)
    {
        Reservoir outputReservoir = Reservoir_default();
        
        Reservoir_update(outputReservoir,
            reservoirData.p_hat * reservoir.contributionWeight * reservoir.sampleCount,
            reservoir.sampleCount, rngState);
        
        vec3 origin = getPositionWs(pixel, centerNormalAndDepth.w);
        vec3 normal = centerNormalAndDepth.xyz;
        
        Reservoir sampledReservoir = Reservoir_fromPackedClamped(restirReservoirsHistory[prevId], RESERVOIR_M_CLAMP);

        vec3 direction;
        float tMax;
        if (reservoirData.triangleLightSource != U32_MAX)
        {
            mat4 lightTransform = inverse(blasInstances[sampledReservoirData.blasInstance].invTransform);
            LightSample reconstructedLightSample = sampleTriangleLight(sampledReservoirData.triangleLightSource, sampledReservoirData.hitUv, lightTransform, origin, 0.0);
            direction = reconstructedLightSample.sampleDirection;
            tMax = reconstructedLightSample.hitT;
        }
        else
        {
            direction = sampleSunDirection(reservoirData.hitUv);
            tMax = SUN_DISTANCE;
        }
        
        if (dot(direction, normal) > 0.0)
        {
            vec3 brdfEval = diffuseBsdfEval(vec3(0.7, 0.7, 0.7)); // TODO: pass basecolor around?
            vec3 brdfContribution = bsdfContribution(brdfEval, normal, direction, 1.0);
            float intensity = lightIntensity(sampledReservoirData.triangleLightSource, sampledReservoirData.blasInstance,
                direction, tMax);
        
            float visibility = 1.0;
            if (constants.unbiased)
            {
                visibility = traceValidationRay(Scene, origin, direction, normal, tMax) ? 0.0 : 1.0;
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
        
        outputReservoir.contributionWeight = (reservoirData.p_hat > 0.0) ? ((1.0 / reservoirData.p_hat) * outputReservoir.weightSum / outputReservoir.sampleCount) : 0.0;

        Reservoir_clampContributionWeight(outputReservoir, RESERVOIR_CONTRIBUTION_CLAMP);

        restirReservoirs[id] = Reservoir_toPacked(outputReservoir);
        restirReservoirData[id] = ReservoirData_toPacked(reservoirData);
    }
}