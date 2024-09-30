#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/color_helpers.shader"
#include "[engine]/shaders/math.shader"

const float MAX_ACCUMULATED_FRAMES = 64.0;
const float DISOCCLUSION_THRESHOLD = 0.3;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D inImage;
layout (BINDING(0, 2), rgba32f) uniform image2D history;
layout (BINDING(0, 3), rgba32f) uniform image2D outHistory;
layout (BINDING(0, 4), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 5), rgba32f) uniform image2D gbufferHistory;
layout (BINDING(0, 6), rgba32f) uniform image2D neGbufferHistory;
layout (BINDING(0, 7), rgba32f) uniform image2D velocity;
layout (BINDING(0, 8), rgba32f) uniform image2D outImage;

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
    bool outOfBounds = (prevPixel.x >= constants.resolution.x || prevPixel.y >= constants.resolution.y || prevPixel.x < 0 || prevPixel.y < 0);
    
    if (outOfBounds)
    {
        return true;
    }

    uint historyBlasInstance;
    vec4 historyNormalAndDepth = getPixelNormalAndDepthHistory(prevPixel, historyBlasInstance);
    vec3 historyPosition = imageLoad(neGbufferHistory, prevPixel).xyz;

    if (historyBlasInstance != currentBlasInstance)
    {
        return true;
    }

    float noXprev1 = abs(dot(historyPosition, currentNormalAndDepth.xyz));
    float noXprev2 = abs(dot(historyPosition, historyNormalAndDepth.xyz));
    float noXprev = max(noXprev1, noXprev2) / currentNormalAndDepth.w;
    float noVprev = noXprev / currentNormalAndDepth.w;
    float relativePlaneDistance = abs(noVprev * historyNormalAndDepth.w - noXprev);
    
    return relativePlaneDistance >= DISOCCLUSION_THRESHOLD;
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec3 current = imageLoad(inImage, pixel).rgb;

    uint currentBlasInstance;
    vec4 currentNormalAndDepth = getPixelNormalAndDepth(pixel, currentBlasInstance);
    if (currentNormalAndDepth.w == 0.0)
    {
        imageStore(outImage, pixel, vec4(current, 1.0));
        imageStore(outHistory, pixel, vec4(0.0));
        return;
    }

    vec2 velocity = imageLoad(velocity, pixel).rg / 100.0;
    ivec2 prevPixel = pixel - ivec2(vec2(constants.resolution) * velocity);

    vec4 history = imageLoad(history, prevPixel);

    bool disoccluded = isDisoccluded(pixel, prevPixel, currentBlasInstance, currentNormalAndDepth);

    if (disoccluded)
    {
        history.w = 0.0;
    }
    else
    {
        history.w += 1.0;
        history.w = min(history.w, MAX_ACCUMULATED_FRAMES);
    }

    vec3 result = mix(history.rgb, current, 1.0 / (1.0 + history.w));

    if (dot(result, current) < 0.5)
    {
        history.w = min(history.w, MAX_ACCUMULATED_FRAMES * 0.5);
        history.w *= 0.3;
        result = mix(history.rgb, current, 1.0 / (1.0 + history.w));
    }

    history.rgb = result;

    imageStore(outImage, pixel, vec4(result, 1.0));
    imageStore(outHistory, pixel, history);
}