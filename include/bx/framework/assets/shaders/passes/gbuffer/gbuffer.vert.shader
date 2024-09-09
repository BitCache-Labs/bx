#include "[engine]/shaders/Language.shader"

layout (location = 0) in vec3 Position;

layout (location = 0) flat out uvec2 Frag_EntityID;

layout (BINDING(0, 0), std140) uniform ConstantBuffer
{
    mat4 ViewProjMtx;
};

layout (BINDING(0, 1), std140) uniform ModelBuffer
{
    mat4 WorldMeshMtx;
    uvec2 EntityID;
};

void main()
{
   gl_Position = ViewProjMtx * WorldMeshMtx * vec4(Position, 1.0);
   Frag_EntityID = EntityID;
}