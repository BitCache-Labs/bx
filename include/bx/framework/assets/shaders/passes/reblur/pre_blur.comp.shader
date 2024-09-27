#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D inImage;
layout (BINDING(0, 2), rgba32f) uniform image2D outImage;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec3 result = vec3(0.0);

    float KERNEL[9] = float[](
        1.0, 2.0, 1.0,
        2.0, 4.0, 2.0,
        1.0, 2.0, 1.0
    );

    #pragma unroll
    for (uint y = 0; y < 3; y++)
    {
        #pragma unroll
        for (uint x = 0; x < 3; x++)
        {
            result += imageLoad(inImage, pixel + ivec2(x - 1, y - 1) * 2).rgb * KERNEL[y * 3 + x];
        }
    }

    result /= 16.0;

    imageStore(outImage, pixel, vec4(result, 1.0));
}