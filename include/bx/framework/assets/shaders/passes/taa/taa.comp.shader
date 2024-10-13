#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/color_helpers.shader"
#include "[engine]/shaders/packing.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 globalResolution;
    uvec2 resolution;
    float historyWeight;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D colorTarget;
layout (BINDING(0, 2), rgba32f) uniform image2D resolvedColorTarget;
layout (BINDING(0, 3), rg16f) uniform image2D velocityTarget;
layout (BINDING(0, 4), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 5), rgba32f) uniform image2D gbufferHistory;
layout (BINDING(0, 6), rgba32f) uniform image2D resolvedColorTargetHistory;

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

// Source: M. Pharr, W. Jakob, and G. Humphreys, Physically Based Rendering, Morgan Kaufmann, 2016.
float mitchell1D(float x, float B, float C)
{
    x = abs(2.0 * x);
    const float oneDivSix = 1.0 / 6.0;

    if (x > 1)
    {
        return ((-B - 6.0 * C) * x * x * x + (6.0 * B + 30.0 * C) * x * x +
                (-12.0 * B - 48.0 * C) * x + (8.0 * B + 24.0 * C)) * oneDivSix;
    }
    else
    {
        return ((12.0 - 9.0 * B - 6.0 * C) * x * x * x +
                (-18.0 + 12.0 * B + 6.0 * C) * x * x +
                (6.0 - 2.0 * B)) * oneDivSix;
    }
}

// Source: https://github.com/playdeadgames/temporal
vec3 clipAabb(vec3 aabbMin, vec3 aabbMax, vec3 histSample)
{
    vec3 center = 0.5 * (aabbMax + aabbMin);
    vec3 extents = 0.5 * (aabbMax - aabbMin);

    vec3 rayToCenter = histSample - center;
    vec3 rayToCenterUnit = rayToCenter.xyz / extents;
    rayToCenterUnit = abs(rayToCenterUnit);
    float rayToCenterUnitMax = max(rayToCenterUnit.x, max(rayToCenterUnit.y, rayToCenterUnit.z));

    if (rayToCenterUnitMax > 1.0)
    {
        return center + rayToCenter / rayToCenterUnitMax;
    }
    else
    {
        return histSample;
    }
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    vec3 current = imageLoad(colorTarget, pixel).rgb;
    vec4 currentNormalAndDepth = getPixelNormalAndDepth(globalPixel);

    if (currentNormalAndDepth.w == 0.0)
    {
        imageStore(resolvedColorTarget, pixel, vec4(current, 1.0));
        return;
    }

    float weightSum = sqr(mitchell1D(0, 0.33, 0.33));
    vec3 reconstructed = current * weightSum;
    vec3 firstMoment = current;
    vec3 secondMoment = sqr(current);

    float sampleCount = 1.0;

    #pragma unroll
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            if (x == 0 && y == 0)
            {
                continue;
            }

            ivec2 samplePixel = pixel + ivec2(x, y);
            if (samplePixel.x < 0 || samplePixel.y < 0 || samplePixel.x >= constants.resolution.x || samplePixel.y >= constants.resolution.y)
            {
                continue;
            }

            vec3 sampleColor = max(imageLoad(colorTarget, samplePixel).rgb, vec3(0.0)); // TODO: clamp required?
            float weight = mitchell1D(x, 0.33, 0.33) * mitchell1D(y, 0.33, 0.33);
            weight *= 1.0 / (1.0 + linearToLuma(sampleColor));

            reconstructed += sampleColor * weight;
            weightSum += weight;

            firstMoment += sampleColor;
            secondMoment += sqr(sampleColor);

            sampleCount += 1.0;
        }
    }

    reconstructed /= max(weightSum, 1e-5);

    vec2 velocity = imageLoad(velocityTarget, globalPixel).rg;
    ivec2 prevPixel = pixel - ivec2(vec2(constants.resolution) * velocity);
    ivec2 globalPrevPixel = globalPixel - ivec2(vec2(constants.globalResolution) * velocity);

    if (prevPixel.x < 0 || prevPixel.y < 0 || prevPixel.x >= constants.resolution.x || prevPixel.y >= constants.resolution.y)
    {
        imageStore(resolvedColorTarget, pixel, vec4(reconstructed, 1.0));
        return;
    }

    vec4 historyNormalAndDepth = getPixelNormalAndDepthHistory(globalPrevPixel);
    vec3 history = imageLoad(resolvedColorTargetHistory, prevPixel).rgb;

    vec3 mean = firstMoment / sampleCount;
    vec3 std = abs(secondMoment - sqr(firstMoment) / sampleCount);
    std /= (sampleCount - 1.0);
    std = sqrt(std);

    vec3 clippedHistory = clipAabb(mean - std, mean + std, history);
    
    float blendWeight = 1.0 - constants.historyWeight;
    
    float currentWeight = saturate(blendWeight * (1.0 / (1.0 + linearToLuma(reconstructed))));
    float historyWeight = saturate((1.0 - blendWeight) * (1.0 / (1.0 + linearToLuma(clippedHistory))));
    vec3 result = (currentWeight * reconstructed + historyWeight * clippedHistory) / (currentWeight + historyWeight);
    result = fixNan(result);
    
    imageStore(resolvedColorTarget, pixel, vec4(result, 1.0));

    //bool depthSimilarity = abs(currentNormalAndDepth.w - historyNormalAndDepth.w) < 0.1;
    //bool normalSimilarity = dot(currentNormalAndDepth.xyz, historyNormalAndDepth.xyz) >= 0.86;
    //if (!depthSimilarity || !normalSimilarity)
    //{
    //    history = current;
    //}
    //else
    //{
    //    vec3 nearColor0 = imageLoad(colorTarget, pixel + ivec2(1, 0)).xyz;
    //    vec3 nearColor1 = imageLoad(colorTarget, pixel + ivec2(0, 1)).xyz;
    //    vec3 nearColor2 = imageLoad(colorTarget, pixel + ivec2(-1, 0)).xyz;
    //    vec3 nearColor3 = imageLoad(colorTarget, pixel + ivec2(0, -1)).xyz;
    //    vec3 boxMin = min(current, min(nearColor0, min(nearColor1, min(nearColor2, nearColor3))));
    //    vec3 boxMax = max(current, max(nearColor0, max(nearColor1, max(nearColor2, nearColor3))));
    //
    //    history = clamp(history, boxMin, boxMax);
    //}
    //
    //vec3 result = mix(current, history, 0.85);
    //
    //imageStore(resolvedColorTarget, pixel, vec4(result, 1.0));
}