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

#include "[engine]/shaders/passes/gbuffer/gbuffer.shader"

const float MAX_SAMPLE_COUNT = 64.0;

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

layout (BINDING(0, 2)) uniform texture2D gbuffer;
layout (BINDING(0, 3)) uniform texture2D gbufferHistory;
layout (BINDING(0, 4)) uniform texture2D reprojection;
layout (BINDING(0, 5), rgba32f) uniform image2D neGbuffer;

layout (BINDING(0, 6)) uniform sampler nearestClampSampler;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.resolution.x * constants.resolution.y) return;
    ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed + 1));

    {
        Reservoir outputReservoir = Reservoir_default();
        ReservoirData outputReservoirData = ReservoirData_default();

        GBufferData centerGBufferData = GBufferData_loadAll(gbuffer, nearestClampSampler, globalPixel, constants.globalResolution);

        vec3 origin = imageLoad(neGbuffer, globalPixel).rgb;
        vec3 normal = centerGBufferData.normal;

        ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
        Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[id]);

        { // Current
            bool currentValid = !GBufferData_isSky(centerGBufferData) && ReservoirData_isValid(reservoirData);
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

        // History
        vec3 reprojection = texture(sampler2D(reprojection, nearestClampSampler), pixelToUv(globalPixel, constants.globalResolution)).rgb;
        vec2 reprojectionUv = reprojection.xy;
        vec2 historyUv = pixelToUv(pixel, constants.resolution) - reprojectionUv;

        if (isUvInBounds(historyUv))
        {
            ivec2 prevPixel = uvToPixel(historyUv, constants.resolution);
            ivec2 prevGlobalPixel = uvToPixel(historyUv, constants.globalResolution);
            uint prevId = prevPixel.y * constants.resolution.x + prevPixel.x;

            GBufferData sampleGBufferData = GBufferData_loadAll(gbufferHistory, nearestClampSampler, prevGlobalPixel, constants.globalResolution);

            if (!GBufferData_isDisoccludedStrict(centerGBufferData, sampleGBufferData))
            {
                ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirDataHistory[prevId]);
                Reservoir sampledReservoir = Reservoir_fromPacked(restirReservoirsHistory[prevId]);

                if (sampledReservoir.sampleCount > MAX_SAMPLE_COUNT * reservoir.sampleCount)
                {
                    sampledReservoir.weightSum *= MAX_SAMPLE_COUNT * reservoir.sampleCount / sampledReservoir.sampleCount;
                    sampledReservoir.sampleCount = MAX_SAMPLE_COUNT * reservoir.sampleCount;
                }

                bool historyValid = !GBufferData_isSky(sampleGBufferData) && ReservoirData_isValid(sampledReservoirData);
                historyValid = historyValid && dot(normal, sampleGBufferData.normal) >= 0.5;

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
                        vec3 brdfEval = diffuseBsdfEval(vec3(1.0)); // TODO: pass basecolor around?
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
        }

        outputReservoir.contributionWeight = (1.0 / max(outputReservoirData.p_hat, RESTIR_EPSILON)) * (outputReservoir.weightSum / max(outputReservoir.sampleCount, RESTIR_EPSILON));
        outputReservoir.contributionWeight = fixNan(outputReservoir.contributionWeight);

        restirReservoirs[id] = Reservoir_toPacked(outputReservoir);
        restirReservoirData[id] = ReservoirData_toPacked(outputReservoirData);
        //restirReservoirsHistory[id] = Reservoir_toPacked(outputReservoir);
        //restirReservoirDataHistory[id] = ReservoirData_toPacked(outputReservoirData);
    }
}