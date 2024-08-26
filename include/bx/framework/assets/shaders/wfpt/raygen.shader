#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/ray_tracing/ray.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 invView;
    mat4 invProj;
    uint width;
    uint height;
} constants;

layout (BINDING(0, 1), std430) writeonly buffer _Rays
{
    PackedRay rays[];
};

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    if (id.x >= constants.width || id.y >= constants.height) return;

    vec2 pixelCenter = vec2(id.x + 0.5, id.y + 0.5);
    vec2 uv = (pixelCenter / vec2(constants.width, constants.height)) * 2.0 - 1.0;
    uv.y = -uv.y;
    vec4 origin = constants.invView * vec4(0.0, 0.0, 0.0, 1.0);
    vec4 target = constants.invProj * vec4(uv, 1.0, 1.0);
    vec4 direction = constants.invView * vec4(normalize(target.xyz), 0.0);

    Ray ray;
    ray.origin = origin.xyz;
    ray.direction = direction.xyz;

    rays[id.y * constants.width + id.x] = packRay(ray);
}