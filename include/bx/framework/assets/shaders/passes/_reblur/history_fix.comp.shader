#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/color_helpers.shader"
#include "[engine]/shaders/sampling.shader"

const int ANTI_FIREFLY_RADIUS = 6;
const float ANTI_FIREFLY_SCALE = 0.5;

const int FAST_HISTORY_RADIUS = 2; // or 1
const float FAST_HISTORY_SCALE = 0.16; // or 2.0

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 globalResolution;
    uvec2 resolution;
    bool antiFirefly;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D inImage;
layout (BINDING(0, 2)) uniform texture2D mippedBlur;
layout (BINDING(0, 3), rgba32f) uniform image2D outHistory;
layout (BINDING(0, 4), rgba32f) uniform image2D outImage;

layout (BINDING(0, 5)) uniform sampler linearClampSampler;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    float frameCount = imageLoad(outHistory, pixel).w;
    bool disoccluded = (frameCount == 0.0);

    vec3 current = imageLoad(inImage, pixel).rgb;

    vec3 result = current;

    if (disoccluded)
    {
        vec2 uv = vec2(pixel) / vec2(constants.resolution);
        float depth = textureLod(sampler2D(mippedBlur, linearClampSampler), uv, 0.0).w;

        #pragma unroll
        for (int i = 3; i >= 1; i--)
        {
            vec4 mippedData = textureLod(sampler2D(mippedBlur, linearClampSampler), uv, i);
            float mippedDepth = mippedData.w;

            if (abs(mippedDepth - depth) < 0.1 && i > 1)
            {
                continue;
            }

            result = mippedData.rgb;
            break;
        }
    }

    float luma = linearToLuma(result);

    if (constants.antiFirefly)
    {
        float m1 = 0.0;
        float m2 = 0.0;

        #pragma unroll
        for (int x = -ANTI_FIREFLY_RADIUS; x <= ANTI_FIREFLY_RADIUS; x++)
        {
            #pragma unroll
            for (int y = -ANTI_FIREFLY_RADIUS; y <= ANTI_FIREFLY_RADIUS; y++)
            {
                if (abs(x) <= 1 && abs(y) <= 1)
                {
                    continue;
                }

                ivec2 samplePixel = pixel + ivec2(x, y);
                samplePixel = mirrorSample(samplePixel, ivec2(constants.resolution));

                float sampleLuma = linearToLuma(imageLoad(inImage, samplePixel).rgb);
                m1 += sampleLuma;
                m2 += sqr(sampleLuma);
            }
        }

        float invNorm = 1.0 / ((ANTI_FIREFLY_RADIUS * 2 + 1) * (ANTI_FIREFLY_RADIUS * 2 + 1) - 3 * 3);
        m1 *= invNorm;
        m2 *= invNorm;

        float sigma = stdDev(m1, m2) * ANTI_FIREFLY_SCALE;
        float clampedLuma = clamp(luma, m1 - sigma, m1 + sigma);
        float lumaFactor = (luma == 0.0) ? 0.0 : (clampedLuma / luma);
        luma = clampedLuma;

        result *= lumaFactor; // TODO: incorrect
    }

    if (false)
    {
        float m1 = 0.0;
        float m2 = 0.0;

        #pragma unroll
        for (int y = -FAST_HISTORY_RADIUS; y <= FAST_HISTORY_RADIUS; y++)
        {
            #pragma unroll
            for (int x = -FAST_HISTORY_RADIUS; x <= FAST_HISTORY_RADIUS; x++)
            {
                if (x == 0 && y == 0)
                {
                    continue;
                }

                ivec2 samplePixel = pixel + ivec2(x, y);
                samplePixel = mirrorSample(samplePixel, ivec2(constants.resolution));

                float sampleLuma = linearToLuma(imageLoad(inImage, samplePixel).rgb);
                m1 += sampleLuma;
                m2 += sqr(sampleLuma);
            }
        }

        float invNorm = 1.0 / ((FAST_HISTORY_RADIUS * 2 + 1) * (FAST_HISTORY_RADIUS * 2 + 1) - 1);
        m1 *= invNorm;
        m2 *= invNorm;

        float sigma = stdDev(m1, m2) * FAST_HISTORY_SCALE;
        float clampedLuma = clamp(luma, m1 - sigma, m1 + sigma);
        clampedLuma = mix(clampedLuma, luma, 1.0 / (1.0 + frameCount));
        float lumaFactor = (luma == 0.0) ? 0.0 : (clampedLuma / luma);
        luma = clampedLuma;

        result *= lumaFactor; // TODO: incorrect
    }

    imageStore(outImage, pixel, vec4(result, 1.0));
    //imageStore(outHistory, pixel, vec4(result, frameCount));
}