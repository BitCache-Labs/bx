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
    float x = 1.0 / constants.srcResolution.x;
    float y = 1.0 / constants.srcResolution.y;

    vec3 a = texture(src, vec2(texCoord.x - 2*x, texCoord.y + 2*y)).rgb;
    vec3 b = texture(src, vec2(texCoord.x,       texCoord.y + 2*y)).rgb;
    vec3 c = texture(src, vec2(texCoord.x + 2*x, texCoord.y + 2*y)).rgb;

    vec3 d = texture(src, vec2(texCoord.x - 2*x, texCoord.y)).rgb;
    vec3 e = texture(src, vec2(texCoord.x,       texCoord.y)).rgb;
    vec3 f = texture(src, vec2(texCoord.x + 2*x, texCoord.y)).rgb;

    vec3 g = texture(src, vec2(texCoord.x - 2*x, texCoord.y - 2*y)).rgb;
    vec3 h = texture(src, vec2(texCoord.x,       texCoord.y - 2*y)).rgb;
    vec3 i = texture(src, vec2(texCoord.x + 2*x, texCoord.y - 2*y)).rgb;

    vec3 j = texture(src, vec2(texCoord.x - x, texCoord.y + y)).rgb;
    vec3 k = texture(src, vec2(texCoord.x + x, texCoord.y + y)).rgb;
    vec3 l = texture(src, vec2(texCoord.x - x, texCoord.y - y)).rgb;
    vec3 m = texture(src, vec2(texCoord.x + x, texCoord.y - y)).rgb;

    vec3 downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;

    imageStore(dst, pixel, vec4(downsample, 1.0));
}