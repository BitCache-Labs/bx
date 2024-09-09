#include "[engine]/shaders/Language.shader"

layout (location = 0) flat in uvec2 Frag_EntityID;

layout (location = 0) out uvec2 Out_Color;

void main()
{
    Out_Color = Frag_EntityID;
}