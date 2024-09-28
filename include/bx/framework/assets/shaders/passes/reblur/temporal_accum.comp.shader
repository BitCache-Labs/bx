#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"

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
layout (BINDING(0, 6), rgba32f) uniform image2D velocity;
layout (BINDING(0, 7), rgba32f) uniform image2D outImage;

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
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec2 velocity = imageLoad(velocity, pixel).rg / 100.0;
    ivec2 prevPixel = pixel - ivec2(vec2(constants.resolution) * velocity);
    if (prevPixel.x >= constants.resolution.x || prevPixel.y >= constants.resolution.y || prevPixel.x < 0 || prevPixel.y < 0)
    {
        prevPixel = pixel;
    }
    uint prevId = prevPixel.y * constants.resolution.x + prevPixel.x;

    vec3 current = imageLoad(inImage, pixel).rgb;
    vec4 history = imageLoad(history, prevPixel);

    vec4 currentNormalAndDepth = getPixelNormalAndDepth(pixel);
    vec4 historyNormalAndDepth = getPixelNormalAndDepthHistory(prevPixel);
    bool validDepth = abs(currentNormalAndDepth.w - historyNormalAndDepth.w) < 0.3 && historyNormalAndDepth.w != 0.0;
    bool validNormals = dot(currentNormalAndDepth.xyz, historyNormalAndDepth.xyz) >= 0.86;

    if (validDepth && validNormals)
    {
        history.w += 1.0;
        history.w = min(history.w, 64.0);
    }
    else
    {
        history.w = 0.0;
    }

    vec3 result = mix(history.rgb, current, 1.0 / (1.0 + history.w));
    history.rgb = result;

    imageStore(outImage, pixel, vec4(result, 1.0));
    imageStore(outHistory, pixel, history);
}