#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/math.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint groupSize;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (BINDING(0, 1), std430) readonly buffer _Count
{
    uint count;
};

layout (BINDING(0, 2), std430) writeonly buffer _IndirectArgs
{
    uint x;
    uint y;
    uint z;
} indirectArgs;

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main()
{
    indirectArgs.x = divCeil(count, constants.groupSize);
    indirectArgs.y = 1;
    indirectArgs.z = 1;
}