#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/wfpt/payload.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint width;
    uint height;
} constants;

layout (BINDING(0, 1), std430) readonly buffer _Payloads
{
    Payload payloads[];
};

layout (BINDING(0, 2), rgba8) uniform image2D OutImage;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    if (id.x >= constants.width || id.y >= constants.height) return;
    uint i = id.y * constants.width + id.x;

    Payload payload = payloads[i];

    imageStore(OutImage, id, vec4(unpackRgb9e5(payload.accumulated), 1.0));
}