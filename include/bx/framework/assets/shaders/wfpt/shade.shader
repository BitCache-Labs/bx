#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/ray_tracing/ray.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint width;
    uint height;
} constants;

layout (BINDING(0, 1), std430) readonly buffer _Rays
{
    Ray rays[];
};
layout(BINDING(0, 2)) readonly buffer _RayCount
{
    uint rayCount;
};

layout (BINDING(0, 3), std430) readonly buffer _Intersections
{
    Intersection intersections[];
};

layout (BINDING(0, 4), rgba8) uniform image2D OutImage;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= rayCount) return;
    uint pid = id; // TODO: pixelmapping

    Intersection intersection = intersections[pid];
    Ray ray = rays[id];

    vec3 color = vec3(0.0);

    if (intersection.t != T_MISS)
    {
        color = vec3(intersection.t * 0.03);
    }
    else
    {

    }

    ivec2 pid2d = ivec2(pid % constants.width, pid / constants.width);
    imageStore(OutImage, pid2d, vec4(color, 1.0));
}