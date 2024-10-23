#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/packing.shader"

layout (location = 0) in vec4 inPositionWs;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoord;
layout (location = 3) flat in uint inBlasInstanceIdx;
layout (location = 4) in vec4 inPositionWsHistory;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outVelocity;

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 viewProj;
    mat4 viewProjHistory;
    mat4 view;
    mat4 viewHistory;
    vec3 viewPos;
    uint _PADDING0;
    vec2 jitter;
    uvec2 resolution;
} constants;

void main()
{
    float oneOverSqrDepth = 1.0 / distance(constants.viewPos, inPositionWs.xyz);
    float packedNormal = uintBitsToFloat(packNormalizedXyz10(inNormal, 0).data);
    float packedTexcoord = uintBitsToFloat(packHalf2x16(inTexcoord));
    float blasInstanceIdx = uintBitsToFloat((uint(gl_FrontFacing) << 31) | inBlasInstanceIdx);

    outColor = vec4(oneOverSqrDepth, packedNormal, packedTexcoord, blasInstanceIdx);

    vec3 positionVsHistory = (constants.view * inPositionWsHistory).xyz;
    vec3 positionVs = (constants.view * inPositionWs).xyz;

    outVelocity = vec4(positionVsHistory - positionVs, 1.0);
}