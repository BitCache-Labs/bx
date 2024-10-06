#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/packing.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
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

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);

    vec2 velocity = imageLoad(velocityTarget, pixel).rg;
    ivec2 prevPixel = pixel - ivec2(vec2(constants.resolution) * velocity);

    vec3 current = imageLoad(colorTarget, pixel).rgb;
    vec3 history = imageLoad(resolvedColorTargetHistory, prevPixel).rgb;

    vec4 currentNormalAndDepth = getPixelNormalAndDepth(pixel);
    vec4 historyNormalAndDepth = getPixelNormalAndDepthHistory(prevPixel);

    bool depthSimilarity = abs(currentNormalAndDepth.w - historyNormalAndDepth.w) < 0.1;
    bool normalSimilarity = dot(currentNormalAndDepth.xyz, historyNormalAndDepth.xyz) >= 0.86;
    if (!depthSimilarity || !normalSimilarity)
    {
        history = current;
    }
    else
    {
        vec3 nearColor0 = imageLoad(colorTarget, pixel + ivec2(1, 0)).xyz;
        vec3 nearColor1 = imageLoad(colorTarget, pixel + ivec2(0, 1)).xyz;
        vec3 nearColor2 = imageLoad(colorTarget, pixel + ivec2(-1, 0)).xyz;
        vec3 nearColor3 = imageLoad(colorTarget, pixel + ivec2(0, -1)).xyz;
        vec3 boxMin = min(current, min(nearColor0, min(nearColor1, min(nearColor2, nearColor3))));
        vec3 boxMax = max(current, max(nearColor0, max(nearColor1, max(nearColor2, nearColor3))));
    
        history = clamp(history, boxMin, boxMax);
    }

    vec3 result = mix(current, history, 0.85);

    imageStore(resolvedColorTarget, pixel, vec4(result, 1.0));
}