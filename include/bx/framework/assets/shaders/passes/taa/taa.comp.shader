#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/math.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D colorTarget;
layout (BINDING(0, 2), rgba32f) uniform image2D resolvedColorTarget;
layout (BINDING(0, 3), rg32f) uniform image2D velocityTarget;
layout (BINDING(0, 4), rgba32f) uniform image2D resolvedColorTargetHistory;

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

    vec3 result = mix(current, history, 0.9);

    imageStore(resolvedColorTarget, pixel, vec4(result, 1.0));
}