#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/packing.shader"

layout (location = 0) in vec4 inPositionWs;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoord;
layout (location = 3) flat in uint inBlasInstanceIdx;

layout (location = 0) out vec4 outColor;

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 viewProj;
    vec3 viewPos;
    uint _PADDING0;
} constants;

void main()
{
    float oneOverSqrDepth = 1.0 / distanceSqr(constants.viewPos, inPositionWs.xyz);
    float packedNormal = uintBitsToFloat(packNormalizedXyz10(inNormal, 0).data);
    float packedTexcoord = uintBitsToFloat(packHalf2x16(inTexcoord));
    float blasInstanceIdx = uintBitsToFloat(inBlasInstanceIdx);

    outColor = vec4(oneOverSqrDepth, packedNormal, packedTexcoord, blasInstanceIdx);
}