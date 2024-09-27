// take noisy illumination
// merge reprojected history sample (weight: normal sim, depth sim, velocity magnitude)
// merge neighbour samples (weight: normal sim, depth sim)
// multiply illumination with brdf eval
// add ambient emissive map
// DONE!

#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/reservoir.shader"

const uint NUM_SPATIAL_SAMPLES = 5;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint seed;
    bool denoise;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 2), rgba32f) uniform image2D gbufferHistory;

layout (BINDING(0, 3), rgba32f) uniform image2D shadeResult;
layout (BINDING(0, 4), rgba32f) uniform image2D shadeResultHistory;

layout (BINDING(0, 5), rgba32f) uniform image2D velocity; // TODO: remove

layout (BINDING(0, 6), rgba32f) uniform image2D outImage;

vec4 getPixelNormalAndDepth(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec4 currentPacked = imageLoad(shadeResult, pixel);
    vec3 currentLighting = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(currentPacked.x)));
    vec3 currentAmbientEmissive = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(currentPacked.y)));
    vec3 currentBaseColorFactor = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(currentPacked.z)));

    vec3 resolvedLighting = currentLighting;
    float lightingSampleCount = 1.0;

    if (constants.denoise)
    {
        vec4 currentNormalAndDepth = getPixelNormalAndDepth(pixel);

        // Spatial reuse
        float radius = (constants.resolution.x / 800.0) * currentNormalAndDepth.w;
        float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed) * 0.5;
        ivec2 pixelSeed = pixel >> 2;
        uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed));
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

            vec4 samplePacked = imageLoad(shadeResult, samplePixel);
            vec3 sampleLighting = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(samplePacked.x)));
            vec3 sampleAmbientEmissive = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(samplePacked.y)));
            vec3 sampleBaseColorFactor = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(samplePacked.z)));

            vec4 sampleNormalAndDepth = getPixelNormalAndDepth(samplePixel);

            bool depthSimilarity = abs(currentNormalAndDepth.w - sampleNormalAndDepth.w) < 0.1;
            bool normalSimilarity = dot(currentNormalAndDepth.xyz, sampleNormalAndDepth.xyz) >= 0.86;
            if (depthSimilarity && normalSimilarity)
            {
                float weight = 1.0;
                resolvedLighting += sampleLighting * weight;
                lightingSampleCount += weight;
            }
        }
    }
    
    resolvedLighting /= lightingSampleCount;

    vec3 resolved = resolvedLighting * currentBaseColorFactor + currentAmbientEmissive;
    imageStore(outImage, pixel, vec4(resolved, 1.0));

    currentPacked.x = uintBitsToFloat(packRgb9e5(resolvedLighting).data);
    imageStore(shadeResult, pixel, currentPacked);
}