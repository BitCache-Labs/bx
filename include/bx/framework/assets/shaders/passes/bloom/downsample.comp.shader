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

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.dstResolution.x || pixel.y >= constants.dstResolution.y) return;

    ivec2 srcPixel = ivec2((vec2(pixel) / vec2(constants.dstResolution)) * vec2(constants.srcResolution));

    vec3 a = imageLoad(src, ivec2(srcPixel.x - 2, srcPixel.y + 2)).rgb;
    vec3 b = imageLoad(src, ivec2(srcPixel.x,     srcPixel.y + 2)).rgb;
    vec3 c = imageLoad(src, ivec2(srcPixel.x + 2, srcPixel.y + 2)).rgb;

    vec3 d = imageLoad(src, ivec2(srcPixel.x - 2, srcPixel.y)).rgb;
    vec3 e = imageLoad(src, ivec2(srcPixel.x,     srcPixel.y)).rgb;
    vec3 f = imageLoad(src, ivec2(srcPixel.x + 2, srcPixel.y)).rgb;
    
    vec3 g = imageLoad(src, ivec2(srcPixel.x - 2, srcPixel.y - 2)).rgb;
    vec3 h = imageLoad(src, ivec2(srcPixel.x,     srcPixel.y - 2)).rgb;
    vec3 i = imageLoad(src, ivec2(srcPixel.x + 2, srcPixel.y - 2)).rgb;
    
    vec3 j = imageLoad(src, ivec2(srcPixel.x - 1, srcPixel.y + 1)).rgb;
    vec3 k = imageLoad(src, ivec2(srcPixel.x + 1, srcPixel.y + 1)).rgb;
    vec3 l = imageLoad(src, ivec2(srcPixel.x - 1, srcPixel.y - 1)).rgb;
    vec3 m = imageLoad(src, ivec2(srcPixel.x + 1, srcPixel.y - 1)).rgb;

    vec3 downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;

    imageStore(dst, pixel, vec4(downsample, 1.0));
}