#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D ambientEmissiveBaseColor;
layout (BINDING(0, 2), rgba32f) uniform image2D denoisedIllumination;
layout (BINDING(0, 3), rgba32f) uniform image2D outImage;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec4 currentPacked = imageLoad(ambientEmissiveBaseColor, pixel);
    vec3 currentAmbient = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(currentPacked.x)));
    vec3 currentEmissive = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(currentPacked.y)));
    vec3 currentBaseColorFactor = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(currentPacked.z)));

    vec3 resolvedLighting = imageLoad(denoisedIllumination, pixel).rgb;

    vec3 resolved = resolvedLighting * currentBaseColorFactor + currentAmbient + currentEmissive;
    imageStore(outImage, pixel, vec4(resolved, 1.0));
}