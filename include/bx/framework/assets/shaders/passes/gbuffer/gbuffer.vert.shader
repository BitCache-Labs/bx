#include "[engine]/shaders/Language.shader"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec4 inColor;
layout (location = 2) in ivec4 inBones;
layout (location = 3) in vec4 inWeights;
layout (location = 4) in vec3 inNormal;
layout (location = 5) in vec3 inTangent;
layout (location = 6) in vec2 inTexcoord;

layout (location = 0) out vec4 outPositionWs;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outTexcoord;
layout (location = 3) flat out uint outBlasInstanceIdx;

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 viewProj;
    vec3 viewPos;
    uint _PADDING0;
} constants;

layout (BINDING(0, 1), std140) uniform _Model
{
    mat4 worldMesh;
    mat4 transInvWorldMesh;
    uint blasInstanceIdx;
    uint _PADDING0;
} model;

void main()
{
    outPositionWs = model.worldMesh * vec4(inPosition, 1.0);
    gl_Position = constants.viewProj * outPositionWs;

    outNormal = (model.transInvWorldMesh * vec4(inNormal, 1.0)).xyz;
    outTexcoord = inTexcoord;
    outBlasInstanceIdx = model.blasInstanceIdx;
}