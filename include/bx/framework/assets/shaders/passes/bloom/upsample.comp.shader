#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/color_helpers.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 srcResolution;
    uvec2 dstResolution;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D src;
layout (BINDING(0, 2), rgba32f) uniform image2D dst;

ivec2 clampSrcCoord(ivec2 x)
{
    return clamp(x, ivec2(0), ivec2(constants.srcResolution) - 1);
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.dstResolution.x || pixel.y >= constants.dstResolution.y) return;

    ivec2 srcPixel = ivec2((vec2(pixel) / vec2(constants.dstResolution)) * vec2(constants.srcResolution));

    uint radius = 1;

    vec3 a = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x - radius, srcPixel.y + radius))).rgb;
    vec3 b = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x,     srcPixel.y + radius))).rgb;
    vec3 c = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x + radius, srcPixel.y + radius))).rgb;

    vec3 d = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x - radius, srcPixel.y))).rgb;
    vec3 e = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x,     srcPixel.y))).rgb;
    vec3 f = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x + radius, srcPixel.y))).rgb;

    vec3 g = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x - radius, srcPixel.y - radius))).rgb;
    vec3 h = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x,     srcPixel.y - radius))).rgb;
    vec3 i = imageLoad(src, clampSrcCoord(ivec2(srcPixel.x + radius, srcPixel.y - radius))).rgb;

    vec3 upsample = e*4.0;
    upsample += (b+d+f+h)*2.0;
    upsample += (a+c+g+i);
    upsample *= 1.0 / 16.0;

    upsample += imageLoad(dst, pixel).rgb;
    imageStore(dst, pixel, vec4(upsample, 1.0));
}