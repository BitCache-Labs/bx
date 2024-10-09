#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/color_helpers.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    float fogStart;
    float fogEnd;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D image;
layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;

vec4 getPixelNormalAndDepth(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec4 normalAndDepth = getPixelNormalAndDepth(pixel);

    vec3 color = imageLoad(image, pixel).rgb;
    float luma = linearToLuma(color);

    float fogIntensity = (constants.fogEnd - normalAndDepth.w) / (constants.fogEnd - constants.fogStart);
    fogIntensity = sqr(1.0 - saturate(fogIntensity));
    fogIntensity *= saturate(1.0 / luma);
    color = mix(color, vec3(0.1), fogIntensity);

    imageStore(image, pixel, vec4(color, 1.0));
}