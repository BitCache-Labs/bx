#ifndef REBLUR_SHARED_H
#define REBLUR_SHARED_H

const float REBLUR_NORMAL_SIMILARITY_THRESHOLD = 0.86;

bool sampleToCurrentSimilarity(vec4 currentNormalAndDepth, vec4 sampleNormalAndDepth, uint currentBlasInstance, uint sampleBlasInstance, out float weight)
{
	float depthWeight = 1.0 - abs(currentNormalAndDepth.w - sampleNormalAndDepth.w);
    bool validDepth = depthWeight > 0.0 && sampleNormalAndDepth.w != 0.0;
    float normalWeight = 1.0 - ((1.0 - dot(currentNormalAndDepth.xyz, sampleNormalAndDepth.xyz)) / (1.0 - REBLUR_NORMAL_SIMILARITY_THRESHOLD));
    bool validNormals = dot(currentNormalAndDepth.xyz, sampleNormalAndDepth.xyz) >= 0.86;
    float blasInstanceWeight = (currentBlasInstance == sampleBlasInstance) ? 1.0 : 0.5;

    weight = depthWeight * blasInstanceWeight * normalWeight;

    return validDepth && validNormals;
}

#endif // REBLUR_SHARED_H