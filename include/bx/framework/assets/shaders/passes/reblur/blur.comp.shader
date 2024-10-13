#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/passes/reblur/shared.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"

//const uint NUM_SPATIAL_SAMPLES = 16;
const int RADIUS = 1;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 globalResolution;
    uvec2 resolution;
    uint seed;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (BINDING(0, 1)) uniform sampler2D inImage;
layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D outHistory;
layout (BINDING(0, 4), rgba32f) uniform image2D outImage;

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
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    float frameCount = imageLoad(outHistory, pixel).w;

    uint currentBlasInstance;
    vec4 currentNormalAndDepth = getPixelNormalAndDepth(globalPixel, currentBlasInstance);

    vec3 result = texture(inImage, vec2(pixel) / vec2(constants.resolution)).rgb;
    float sampleCount = 1.0;

    if (currentNormalAndDepth.w == 0.0)
    {
        imageStore(outImage, pixel, vec4(result, 1.0));
        imageStore(outHistory, pixel, vec4(fixNan(result), frameCount));
        return;
    }

    float animatedNoise = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 3 + 1) * 0.5;

    #pragma unroll
    for (int x = -RADIUS; x <= RADIUS; x++)
    {
        #pragma unroll
        for (int y = -RADIUS; y <= RADIUS; y++)
        {
            if (x == 0 && y == 0)
            {
                continue;
            }

            ivec2 offset = ivec2(x, y);
            uint flatOffset = offset.y * constants.resolution.x + offset.x;
            ivec2 samplePixel = pixel + offset;
            samplePixel = mirrorSample(samplePixel, ivec2(constants.resolution));
            ivec2 globalSamplePixel = rescaleResolution(samplePixel, constants.resolution, constants.globalResolution);
            
            uint sampleBlasInstance;
            vec4 sampleNormalAndDepth = getPixelNormalAndDepth(globalSamplePixel, sampleBlasInstance);
            
            float weight;
            if (sampleToCurrentSimilarity(currentNormalAndDepth, sampleNormalAndDepth, currentBlasInstance, sampleBlasInstance, weight))
            {
                float lod = (1.0 / (1.0 + frameCount)) * 2.0 + 1.0 + animatedNoise;

                result += textureLod(inImage, vec2(samplePixel) / vec2(constants.resolution), lod).rgb * weight;
                sampleCount += weight;
            }
        }
    }

    result /= sampleCount;

    imageStore(outImage, pixel, vec4(result, 1.0));
    imageStore(outHistory, pixel, vec4(fixNan(result), frameCount));
}