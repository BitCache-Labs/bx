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

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/ray_tracing/light_picking.shader"
#include "[engine]/shaders/random.shader"

#include "[engine]/shaders/passes/gbuffer/gbuffer.shader"

const uint NUM_SPATIAL_SAMPLES = 5;

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 invView;
    mat4 invProj;
    uvec2 globalResolution;
    uvec2 resolution;
    uint seed;
    uint spatialIndex;
    bool unbiased;
    uint _PADDING0;
} constants;

layout (BINDING(0, 1)) uniform texture2D gbuffer;

layout(BINDING(0, 2)) uniform accelerationStructureEXT Scene;
layout (BINDING(0, 3), rgba32f) uniform image2D neGbuffer;

layout (BINDING(0, 4)) uniform sampler nearestClampSampler;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.resolution.x * constants.resolution.y) return;
    ivec2 pixel = ivec2(int(id % constants.resolution.x), int(id / constants.resolution.x));
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);
    
    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));
    
    ReservoirData reservoirData = ReservoirData_fromPacked(restirReservoirData[id]);
    Reservoir reservoir = Reservoir_fromPacked(restirReservoirs[id]);
    
    GBufferData centerGBufferData = GBufferData_loadAll(gbuffer, nearestClampSampler, globalPixel, constants.globalResolution);
    vec3 origin = imageLoad(neGbuffer, globalPixel).rgb;
    vec3 normal = centerGBufferData.normal;
    
    Reservoir outputReservoir = Reservoir_default();
    ReservoirData outputReservoirData = ReservoirData_default();

    if (!GBufferData_isSky(centerGBufferData))
    {
        if (ReservoirData_isValid(reservoirData))
        {
            if (Reservoir_update(outputReservoir,
                reservoirData.p_hat * reservoir.contributionWeight * reservoir.sampleCount,
                reservoir.sampleCount, rngState))
            {
                outputReservoirData = reservoirData;
            }
        }
        
        float radius = (30.0 / 1920.0) * float(constants.resolution.x);
        radius *= (constants.spatialIndex == 0) ? 4.0 : 2.5;
        float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 3 + constants.spatialIndex);
        
        ivec2 pixelSeed = (constants.spatialIndex == 0) ? (pixel >> 2) : (pixel >> 1);
        uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed * 3 + constants.spatialIndex));
        float samplingAngleOffset = angleSeed * (1.0 / float(0xffffffffU)) * TWO_PI;
        
        #pragma unroll
        for (uint i = 0; i < NUM_SPATIAL_SAMPLES; i++)
        {
            float angle = float(i) * GOLDEN_ANGLE + samplingAngleOffset;
            float currentRadius = pow(float(i) / NUM_SPATIAL_SAMPLES, 0.5) * radius + samplingRadiusOffset;
            
            ivec2 offset = ivec2(currentRadius * vec2(cos(angle), sin(angle)));
            ivec2 samplePixel = pixel + offset;

            if (!isPixelInBounds(samplePixel, constants.resolution))
            {
                continue;
            }

            uint sampleId = samplePixel.y * constants.resolution.x + samplePixel.x;
            ivec2 globalSamplePixel = rescaleResolution(samplePixel, constants.resolution, constants.globalResolution);

            ReservoirData sampledReservoirData = ReservoirData_fromPacked(restirReservoirData[sampleId]);
        
            GBufferData sampleGBufferData = GBufferData_loadAll(gbuffer, nearestClampSampler, globalSamplePixel, constants.globalResolution);
            if (GBufferData_isSky(sampleGBufferData) || !ReservoirData_isValid(sampledReservoirData))
            {
                continue;
            }
        
            //float normalWeight = normalSimilarity(centerGBufferData.normal, sampleGBufferData.normal, 16.0);
            //float depthWeight = depthSimilarity(centerGBufferData.distance, sampleGBufferData.distance, 1.0);
            //float weight = normalWeight * depthWeight;
            //if (weight < 0.01)
            //{
            //    continue;
            //}
            
            Reservoir sampledReservoir = Reservoir_fromPacked(restirReservoirs[sampleId]);
        
            vec3 direction;
            float tMax;
            float pdf = 1.0;
            if (sampledReservoirData.triangleLightSource != U32_MAX)
            {
                mat4 lightTransform = blasInstances[sampledReservoirData.blasInstance].transform;
                LightSample reconstructedLightSample = sampleTriangleLight(sampledReservoirData.triangleLightSource, sampledReservoirData.hitUv, lightTransform, origin, 0.0);
                direction = reconstructedLightSample.sampleDirection;
                tMax = reconstructedLightSample.hitT;
                pdf = reconstructedLightSample.pdf;
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
            else
            {
                sampledReservoirData.p_hat = 0.0;
            }

            if (Reservoir_update(outputReservoir,
                sampledReservoirData.p_hat * sampledReservoir.contributionWeight * sampledReservoir.sampleCount,
                sampledReservoir.sampleCount, rngState))
            {
                outputReservoirData = sampledReservoirData;
            }
        }
                
        outputReservoir.contributionWeight = (1.0 / max(outputReservoirData.p_hat, RESTIR_EPSILON)) * (outputReservoir.weightSum / max(outputReservoir.sampleCount, RESTIR_EPSILON));
        outputReservoir.contributionWeight = fixNan(outputReservoir.contributionWeight);

        outRestirReservoirs[id] = Reservoir_toPacked(outputReservoir);
        outRestirReservoirData[id] = ReservoirData_toPacked(outputReservoirData);

        if (constants.spatialIndex == 1 && ReservoirData_isValid(outputReservoirData) && outputReservoir.contributionWeight != 0.0)
        {
            restirReservoirsHistory[id] = Reservoir_toPacked(outputReservoir);
            restirReservoirDataHistory[id] = ReservoirData_toPacked(outputReservoirData);
        }
    }
}