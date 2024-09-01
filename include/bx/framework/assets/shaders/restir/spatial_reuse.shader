#include "[engine]/shaders/Language.shader"

#define RESTIR_BINDINGS
#include "[engine]/shaders/ray_tracing/restir.shader"

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    if (id.x >= constants.width || id.y >= constants.height) return;
    uint i = id.y * constants.width + id.x;


}