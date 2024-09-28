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
layout (BINDING(0, 4), rgba32f) uniform image2D outImage;

vec3 historyMip(ivec2 pixel, uint mip)
{
    return imageLoad(history, pixel / int(pow(2, mip))).rgb;
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    bool disoccluded = (imageLoad(outHistory, pixel).w == 0.0);

    vec3 current = imageLoad(inImage, pixel).rgb;

    vec3 result = vec3(0.0);//current;

    if (disoccluded)
    {
        vec3 historyFix = historyMip(pixel, 1);

        result = mix(historyFix, result, 0.5);
    }

    imageStore(outImage, pixel, vec4(result, 1.0));
}