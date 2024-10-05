#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/passes/reblur/shared.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"

const uint NUM_SPATIAL_SAMPLES = 8;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint seed;
    uint _PADDING0;
} constants;

layout (BINDING(0, 1), r32f) uniform image2D inImage;
layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D outHistory;
layout (BINDING(0, 4), r32f) uniform image2D outImage;

vec4 getPixelNormalAndDepth(ivec2 pixel, out uint blasInstance)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    blasInstance = floatBitsToUint(gbufferData.a);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    float frameCount = imageLoad(outHistory, pixel).w;

    uint currentBlasInstance;
    vec4 currentNormalAndDepth = getPixelNormalAndDepth(pixel, currentBlasInstance);

    vec3 result = unpackRgb9e5FromFloatBits(imageLoad(inImage, pixel).r);
    float sampleCount = 1.0;

    if (currentNormalAndDepth.w == 0.0)
    {
        imageStore(outImage, pixel, vec4(uintBitsToFloat(packRgb9e5(result).data)));
        imageStore(outHistory, pixel, vec4(fixNan(result), frameCount));
        return;
    }

    float screenRadius = (constants.resolution.x / 1920.0) * 10.0;
    float radius = screenRadius;
    float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 3 + 1) * 0.5;
    ivec2 pixelSeed = pixel >> 3;
    uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed * 3 + 1));
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
    
        uint sampleBlasInstance;
        vec4 sampleNormalAndDepth = getPixelNormalAndDepth(samplePixel, sampleBlasInstance);
    
        float weight;
        if (sampleToCurrentSimilarity(currentNormalAndDepth, sampleNormalAndDepth, currentBlasInstance, sampleBlasInstance, weight))
        {
            result += unpackRgb9e5FromFloatBits(imageLoad(inImage, samplePixel).r) * weight;
            sampleCount += weight;
        }
    }

    result /= sampleCount;

    imageStore(outImage, pixel, vec4(uintBitsToFloat(packRgb9e5(result).data)));
    imageStore(outHistory, pixel, vec4(fixNan(result), frameCount));
}