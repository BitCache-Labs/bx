#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/color_helpers.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D image;

vec3 aces(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 gammaCorrect(vec3 x, float gamma)
{
    return pow(x, vec3(1.0 / gamma));
}

vec3 colorCorrect(vec3 color)
{
    vec3 linearHdrColor = color;

    // acesg -> acescct (matrix mult)

    // 2 luts, 1d & 3d textures
    // acescct , apply output transform (monitor dependant)
    
    vec3 sdrColor = aces(linearHdrColor);
    sdrColor = gammaCorrect(sdrColor, 2.2);

    return sdrColor;
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    
    vec3 result = colorCorrect(imageLoad(image, pixel).rgb);

    imageStore(image, pixel, vec4(result, 1.0));
}