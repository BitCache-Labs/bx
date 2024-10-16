#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/color_helpers.shader"
#include "[engine]/shaders/math.shader"

const float MAX_ACCUMULATED_FRAMES = 24.0;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 globalResolution;
    uvec2 resolution;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D inImage;
layout (BINDING(0, 2), rgba32f) uniform image2D history;
layout (BINDING(0, 3), rgba32f) uniform image2D outHistory;
layout (BINDING(0, 4), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 5), rgba32f) uniform image2D gbufferHistory;
layout (BINDING(0, 6), rgba32f) uniform image2D neGbufferHistory;
layout (BINDING(0, 7), rg16f) uniform image2D velocityTarget;
layout (BINDING(0, 8), rgba32f) uniform image2D variance;
layout (BINDING(0, 9), rgba32f) uniform image2D outVariance;
layout (BINDING(0, 10), rgba32f) uniform image2D outImage;

vec4 getPixelNormalAndDepth(ivec2 pixel, out uint blasInstance)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    blasInstance = floatBitsToUint(gbufferData.a);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

vec4 getPixelNormalAndDepthHistory(ivec2 pixel, out uint blasInstance)
{
    vec4 gbufferData = imageLoad(gbufferHistory, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    blasInstance = floatBitsToUint(gbufferData.a);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

bool isDisoccluded(ivec2 pixel, ivec2 prevPixel, uint currentBlasInstance, vec4 currentNormalAndDepth)
{
    bool outOfBounds = (prevPixel.x >= constants.globalResolution.x || prevPixel.y >= constants.globalResolution.y || prevPixel.x < 0 || prevPixel.y < 0);
    
    if (outOfBounds)
    {
        return true;
    }

    uint historyBlasInstance;
    vec4 historyNormalAndDepth = getPixelNormalAndDepthHistory(prevPixel, historyBlasInstance);

    if (historyBlasInstance != currentBlasInstance)
    {
        return true;
    }

    vec3 centerNormal = currentNormalAndDepth.xyz;
    vec3 sampleNormal = historyNormalAndDepth.xyz;
    float normalWeight = normalSimilarity(centerNormal, sampleNormal, 128.0);

    float centerDepth = currentNormalAndDepth.w;
    float sampleDepth = historyNormalAndDepth.w;
    float depthWeight = depthSimilarity(centerDepth, sampleDepth, 1.0);

    float weight = normalWeight * depthWeight;
    return weight < 0.01;
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    vec3 current = imageLoad(inImage, pixel).rgb;

    uint currentBlasInstance;
    vec4 currentNormalAndDepth = getPixelNormalAndDepth(globalPixel, currentBlasInstance);
    if (currentNormalAndDepth.w == 0.0)
    {
        imageStore(outImage, pixel, vec4(current, 1.0));
        imageStore(outHistory, pixel, vec4(0.0));
        return;
    }

    vec2 velocity = imageLoad(velocityTarget, globalPixel).rg;
    ivec2 prevPixel = pixel - ivec2(vec2(constants.resolution) * velocity);
    ivec2 globalPrevPixel = globalPixel - ivec2(vec2(constants.globalResolution) * velocity);

    vec4 history = imageLoad(history, prevPixel);

    bool disoccluded = isDisoccluded(globalPixel, globalPrevPixel, currentBlasInstance, currentNormalAndDepth);
    if (disoccluded)
    {
        history.w = 0.0;
    }
    else
    {
        history.w = min(history.w + 1.0, MAX_ACCUMULATED_FRAMES);
    }

    float alpha = 1.0 / (1.0 + history.w);
    vec3 result = mix(history.rgb, current, alpha);
    history.rgb = result;

    vec2 momentsHistory = imageLoad(variance, prevPixel).gb;

    vec2 moments;
    moments.x = linearToLuma(current);
    moments.y = sqr(moments.x);
    moments = mix(momentsHistory, moments, alpha);

    float newVariance = max(moments.y - sqr(moments.x), 0.0);

    imageStore(outImage, pixel, vec4(result, 1.0));
    imageStore(outHistory, pixel, history);
    imageStore(outVariance, pixel, vec4(newVariance, moments, 0.0));
}