#ifndef TANGENT_SPACE_TRIG_HELPERS_H
#define TANGENT_SPACE_TRIG_HELPERS_H

#include "[engine]/shaders/math.shader"

float cosThetaTangentSpace(vec3 wTangentSpace)
{
    return wTangentSpace.z;
}

float cos2ThetaTangentSpace(vec3 wTangentSpace)
{
    return sqr(wTangentSpace.z);
}

float absCosThetaTangentSpace(vec3 wTangentSpace)
{
    return abs(wTangentSpace.z);
}

float sin2ThetaFromCos2Theta(float cos2Theta)
{
    return max(0.0, 1.0 - cos2Theta);
}

float sin2ThetaTangentSpace(vec3 wTangentSpace)
{
    return max(0.0, 1.0 - cos2ThetaTangentSpace(wTangentSpace));
}

float sin2ThetaTangentSpaceIntermediate(vec3 wTangentSpace, out float cos2Theta)
{
    cos2Theta = cos2ThetaTangentSpace(wTangentSpace);
    return max(0.0, 1.0 - cos2Theta);
}

float sinThetaTangentSpace(vec3 wTangentSpace)
{
    return sqrt(sin2ThetaTangentSpace(wTangentSpace));
}

float tanThetaTangentSpace(vec3 wTangentSpace)
{
    return sinThetaTangentSpace(wTangentSpace) / cosThetaTangentSpace(wTangentSpace);
}

float tan2ThetaTangentSpace(vec3 wTangentSpace)
{
    float cos2Theta;
    float sin2Theta = sin2ThetaTangentSpaceIntermediate(wTangentSpace, cos2Theta);
    return sin2Theta / cos2Theta;
}

float tan2ThetaTangentSpaceIntermediate(vec3 wTangentSpace, out float cos2Theta)
{
    float sin2Theta = sin2ThetaTangentSpaceIntermediate(wTangentSpace, cos2Theta);
    return sin2Theta / cos2Theta;
}

float cosPhiTangentSpace(vec3 wTangentSpace)
{
    float sinTheta = sinThetaTangentSpace(wTangentSpace);
    return (sinTheta == 0.0) ? 1.0 : clamp(wTangentSpace.x / sinTheta, -1.0, 1.0);
}

float sinPhiTangentSpace(vec3 wTangentSpace)
{
    float sinTheta = sinThetaTangentSpace(wTangentSpace);
    return (sinTheta == 0.0) ? 0.0 : clamp(wTangentSpace.y / sinTheta, -1.0, 1.0);
}

#endif // TANGENT_SPACE_TRIG_HELPERS_H