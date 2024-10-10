#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/color_helpers.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 srcResolution;
    uvec2 dstResolution;
} constants;

layout (BINDING(0, 1)) uniform sampler2D src;
layout (BINDING(0, 2), rgba32f) uniform image2D dst;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.dstResolution.x || pixel.y >= constants.dstResolution.y) return;

    vec2 texCoord = vec2(pixel) / vec2(constants.dstResolution);

    float radius = 3.0;
    float x = radius / constants.srcResolution.x;
    float y = radius / constants.srcResolution.y;

    vec3 a = texture(src, vec2(texCoord.x - x, texCoord.y + y)).rgb;
    vec3 b = texture(src, vec2(texCoord.x,     texCoord.y + y)).rgb;
    vec3 c = texture(src, vec2(texCoord.x + x, texCoord.y + y)).rgb;

    vec3 d = texture(src, vec2(texCoord.x - x, texCoord.y)).rgb;
    vec3 e = texture(src, vec2(texCoord.x,     texCoord.y)).rgb;
    vec3 f = texture(src, vec2(texCoord.x + x, texCoord.y)).rgb;

    vec3 g = texture(src, vec2(texCoord.x - x, texCoord.y - y)).rgb;
    vec3 h = texture(src, vec2(texCoord.x,     texCoord.y - y)).rgb;
    vec3 i = texture(src, vec2(texCoord.x + x, texCoord.y - y)).rgb;

    vec3 upsample = e*4.0;
    upsample += (b+d+f+h)*2.0;
    upsample += (a+c+g+i);
    upsample *= 1.0 / 16.0;

    upsample += imageLoad(dst, pixel).rgb;
    imageStore(dst, pixel, vec4(upsample, 1.0));
}