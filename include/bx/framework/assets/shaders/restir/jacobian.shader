#ifndef JACOBIAN_H
#define JACOBIAN_H

const float JACOBIAN_REJECTION = 1e-2;

float jacobianDiffuse(vec3 surfaceNormal, vec3 incidentDirX, vec3 incidentDirY, float squaredDistX, float squaredDistY)
{
    float cosThetaY = dot(surfaceNormal, incidentDirY);
    float cosThetaX = dot(surfaceNormal, incidentDirX);

    float cosThetaRatio = cosThetaY / cosThetaX;
    float hitDistanceRatio = squaredDistX / squaredDistY;

    float jacobian = cosThetaRatio * hitDistanceRatio;

    if (jacobian < JACOBIAN_REJECTION || cosThetaY <= 0.0)
    {
        return 0.0;
    }

    return 1.0 / jacobian;
}

#endif // JACOBIAN_H