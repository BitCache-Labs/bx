#ifndef SAMPLING_H
#define SAMPLING_H

#include "[engine]/shaders/math.shader"

float balanceHeuristic(float pdf0, float pdf1)
{
    float combined = pdf0 + pdf1;
    return pdf0 / combined;
}

vec3 getCosineHemisphereSample(vec2 uv)
{
    float phi = TWO_PI * uv.x;
    float sin_theta = sqrt(1.0 - uv.y);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    return vec3(
        sin_phi * sin_theta,
        cos_phi * sin_theta,
        safeSqrt(uv.y)
    ); 
}

vec3 getUniformHemisphereSample(vec2 uv)
{
	float phi = TWO_PI * uv.x;
    float r = sqrt(1.0 - uv.y * uv.y);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    return vec3(
        r * cos_phi,
        r * sin_phi,
        uv.y
    );
}

vec2 getUniformDiskSample(vec2 uv)
{
    float phi = TWO_PI * uv.x;
    float theta = acos(1.0 - 2.0 * uv.y);

    float sin_phi = sin(phi);
    float cos_phi = cos(phi);
    float sin_theta = sin(theta);

    return vec2(
        sin_theta * cos_phi,
        sin_theta * sin_phi
    );
}

vec3 getUniformSphereSample(vec2 uv)
{
    float phi = TWO_PI * uv.x;
    float theta = acos(1.0 - 2.0 * uv.y);

    float sin_phi = sin(phi);
    float cos_phi = cos(phi);
    float sin_theta = sin(theta);
    float cos_theta = cos(theta);

    return vec3(
        sin_theta * cos_phi,
        sin_theta * sin_phi,
        cos_theta
    );
}

ivec2 mirrorSample(ivec2 samplePos, ivec2 resolution)
{
    ivec2 resMinusOne = resolution - 1;
    ivec2 result = abs(samplePos);

    if (samplePos.x > resMinusOne.x)
    {
        result.x = resMinusOne.x - (result.x - resMinusOne.x);
    }
    if (samplePos.y > resMinusOne.y)
    {
        result.y = resMinusOne.y - (result.y - resMinusOne.y);
    }

    return result;
}

// from https://stackoverflow.com/a/2660181
vec3 perturbDirectionVector(vec2 uv, vec3 direction, float angle)
{
    float h = cos(angle);

    float phi = 2.0 * PI * uv.x;

    float z = h + (1.0 - h) * uv.y;
    float sinT = sqrt(1.0 - z * z);

    float x = cos(phi) * sinT;
    float y = sin(phi) * sinT;

    vec3 bitangent = getPerpendicularVector(direction);
    vec3 tangent = cross(bitangent, direction);

    return bitangent * x + tangent * y + direction * z;
}

// https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
float interleavedGradientNoise(vec2 pos)
{
    return fract(52.9829189 * fract(0.06711056 * pos.x + 0.00583715 * pos.y));
}

// https://blog.demofox.org/2022/01/01/interleaved-gradient-noise-a-different-kind-of-low-discrepancy-sequence/
float interleavedGradientNoiseAnimated(uvec2 pos, uint frame)
{
    uint id = frame % 64;
    float x = float(pos.x) + 5.588238 * float(id);
    float y = float(pos.y) + 5.588238 * float(id);
    return interleavedGradientNoise(vec2(x, y));
}

// https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
vec3 sampleTextureCatmullRomLod(texture2D t, sampler s, float lod, vec2 uv, vec2 texSize)
{
    vec2 samplePos = uv * texSize;
    vec2 texPos1 = floor(samplePos - 0.5) + 0.5;

    vec2 f = samplePos - texPos1;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / (w1 + w2);

    vec2 texPos0 = texPos1 - 1.0;
    vec2 texPos3 = texPos1 + 2.0;
    vec2 texPos12 = texPos1 + offset12;

    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;

    vec3 result = vec3(0.0);
    result += textureLod(sampler2D(t, s), vec2(texPos0.x, texPos0.y), lod).xyz * w0.x * w0.y;
    result += textureLod(sampler2D(t, s), vec2(texPos12.x, texPos0.y), lod).xyz * w12.x * w0.y;
    result += textureLod(sampler2D(t, s), vec2(texPos3.x, texPos0.y), lod).xyz * w3.x * w0.y;
    
    result += textureLod(sampler2D(t, s), vec2(texPos0.x, texPos12.y), lod).xyz * w0.x * w12.y;
    result += textureLod(sampler2D(t, s), vec2(texPos12.x, texPos12.y), lod).xyz * w12.x * w12.y;
    result += textureLod(sampler2D(t, s), vec2(texPos3.x, texPos12.y), lod).xyz * w3.x * w12.y;

    result += textureLod(sampler2D(t, s), vec2(texPos0.x, texPos3.y), lod).xyz * w0.x * w3.y;
    result += textureLod(sampler2D(t, s), vec2(texPos12.x, texPos3.y), lod).xyz * w12.x * w3.y;
    result += textureLod(sampler2D(t, s), vec2(texPos3.x, texPos3.y), lod).xyz * w3.x * w3.y;
    
    return result;
}

vec3 sampleTextureCatmullRom(texture2D t, sampler s, vec2 uv, vec2 texSize)
{
    return sampleTextureCatmullRomLod(t, s, 0.0, uv, texSize);
}

#endif // SAMPLING_H