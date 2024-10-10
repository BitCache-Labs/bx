#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/color_helpers.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    float intensity;
    uint _PADDING0;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D src;
layout (BINDING(0, 2), rgba32f) uniform image2D dst;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec3 bloom = imageLoad(src, pixel).rgb;
    vec3 current = imageLoad(dst, pixel).rgb;

    vec3 result = mix(current, bloom, constants.intensity);
    imageStore(dst, pixel, vec4(result, 1.0));
}