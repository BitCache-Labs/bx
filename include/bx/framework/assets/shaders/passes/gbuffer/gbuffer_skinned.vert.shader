#include "[engine]/shaders/Language.shader"

const uint MAX_BONES = 100;

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
layout (location = 4) out vec4 outPositionHistory;
layout (location = 5) out vec4 outPosition;

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 viewProj;
    mat4 viewProjHistory;
    vec3 viewPos;
    uint _PADDING0;
} constants;

layout (BINDING(0, 1), std140) uniform _Model
{
    mat4 worldMesh;
    mat4 worldMeshHistory;
    mat4 transInvWorldMesh;
    uint blasInstanceIdx;
    uint _PADDING0;
} model;

layout (BINDING(0, 2), std140) uniform _Bones
{
    mat4 bones[MAX_BONES];
};

layout (BINDING(0, 3), std140) uniform _BonesHistory
{
    mat4 bonesHistory[MAX_BONES];
};

void main()
{
    mat4 matrix = mat4(0);
    for (uint i = 0; i < 4; i++)
    {
        matrix += inWeights[i] * bones[inBones[i]];
    }
    vec3 skinnedPosition = (matrix * vec4(inPosition, 1.0)).xyz;

    matrix = mat4(0);
    for (uint i = 0; i < 4; i++)
    {
        matrix += inWeights[i] * bonesHistory[inBones[i]];
    }
    vec3 skinnedPositionHistory = (matrix * vec4(inPosition, 1.0)).xyz;

    outPositionWs = model.worldMesh * vec4(skinnedPosition, 1.0);
    outPosition = constants.viewProj * outPositionWs;
    gl_Position = outPosition;// ???

    outPositionHistory = constants.viewProjHistory * model.worldMeshHistory * vec4(skinnedPositionHistory, 1.0);

    outNormal = (model.transInvWorldMesh * vec4(inNormal, 1.0)).xyz;
    outTexcoord = inTexcoord;
    outBlasInstanceIdx = model.blasInstanceIdx;
}