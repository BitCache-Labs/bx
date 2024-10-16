#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/color_helpers.shader"

const float PHI_COLOR = 10.0;
const float PHI_NORMAL = 128.0;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 globalResolution;
    uvec2 resolution;
    uint stepSize;
    bool writeHistory;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D inImage;
layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D variance;
layout (BINDING(0, 4), rgba32f) uniform image2D outImage;
layout (BINDING(0, 5), rgba32f) uniform image2D outHistory;

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
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    vec4 currentNormalAndDepth = getPixelNormalAndDepth(globalPixel);

    vec3 result = imageLoad(inImage, pixel).rgb;

    if (currentNormalAndDepth.w == 0.0)
    {
        imageStore(outImage, pixel, vec4(result, 1.0));
        return;
    }

    float centerVariance = imageLoad(variance, pixel).r;

    vec3 sum = vec3(0.0);
    float weightSum = 0.0;

    vec3 centerColor = result;
    vec3 centerNormal = currentNormalAndDepth.xyz;
    float centerDepth = currentNormalAndDepth.w;
    float centerLuminance = linearToLuma(centerColor);

    float phiLuminance = PHI_COLOR * sqrt(max(centerVariance + 0.001, 0.0));
    float phiDepth = max(centerDepth, 1e-6) * float(constants.stepSize);
    
    #pragma unroll
    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            float samplePhiDepth = phiDepth * length(vec2(x, y));

            ivec2 samplePixel = pixel + ivec2(x, y) * int(constants.stepSize);
            ivec2 globalSamplePixel = rescaleResolution(samplePixel, constants.resolution, constants.globalResolution);
    
            vec3 sampleColor = imageLoad(inImage, samplePixel).rgb;
            float sampleLuminance = linearToLuma(sampleColor);
            vec4 sampleNormalAndDepth = getPixelNormalAndDepth(globalSamplePixel);
            vec3 sampleNormal = sampleNormalAndDepth.xyz;
            float sampleDepth = sampleNormalAndDepth.w;
    
            float normalWeight = normalSimilarity(centerNormal, sampleNormal, PHI_NORMAL);
            float illuminanceDepthWeight = luminanceAndDepthSimilarity(centerLuminance, centerDepth, sampleLuminance, sampleDepth, phiLuminance, samplePhiDepth);

            float weight = normalWeight * illuminanceDepthWeight;
            sum += sampleColor * weight;
            weightSum += weight;
        }
    }
    sum /= weightSum;

    imageStore(outImage, pixel, vec4(sum, 1.0));

    if (constants.writeHistory)
    {
        vec4 currentHistory = imageLoad(outHistory, pixel);
        imageStore(outHistory, pixel, vec4(sum, currentHistory.w));
    }
}