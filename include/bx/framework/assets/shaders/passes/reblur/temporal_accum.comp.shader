#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/color_helpers.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/sampling.shader"

#include "[engine]/shaders/passes/gbuffer/gbuffer.shader"

const float MAX_ACCUMULATED_FRAMES = 14.0;

const int ANTI_FIREFLY_RADIUS = 6;
const float ANTI_FIREFLY_SCALE = 0.5;

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
layout (BINDING(0, 2)) uniform texture2D inHistory;
layout (BINDING(0, 3), rgba32f) uniform image2D outHistory;
layout (BINDING(0, 4)) uniform texture2D gbuffer;
layout (BINDING(0, 5)) uniform texture2D gbufferHistory;
layout (BINDING(0, 7)) uniform texture2D velocityTarget;
layout (BINDING(0, 8), rgba32f) uniform image2D variance;
layout (BINDING(0, 9), rgba32f) uniform image2D outVariance;
layout (BINDING(0, 10), rgba32f) uniform image2D outImage;

layout (BINDING(0, 11)) uniform sampler nearestClampSampler;
layout (BINDING(0, 12)) uniform sampler linearClampSampler;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    vec3 current = imageLoad(inImage, pixel).rgb;

    if (constants.antiFirefly)
    {
        float luma = linearToLuma(current);

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

        current *= lumaFactor; // TODO: incorrect
    }

    GBufferData currentGBufferData = GBufferData_loadAll(gbuffer, nearestClampSampler, globalPixel, constants.globalResolution);

    if (GBufferData_isSky(currentGBufferData))
    {
        imageStore(outImage, pixel, vec4(current, 1.0));
        imageStore(outHistory, pixel, vec4(0.0));
        return;
    }

    //vec2 velocity = getVelocity(velocityTarget, nearestClampSampler, globalPixel, constants.globalResolution);
    //ivec2 prevPixel = pixel - ivec2(vec2(constants.resolution) * velocity);
    //ivec2 globalPrevPixel = globalPixel - ivec2(vec2(constants.globalResolution) * velocity);
    
    vec2 velocity = getVelocity(velocityTarget, nearestClampSampler, globalPixel, constants.globalResolution);
    vec2 historyUv = pixelToUv(pixel, constants.resolution) - velocity;
    ivec2 historyPixel = uvToPixel(historyUv, constants.resolution);
    ivec2 historyGlobalPixel = uvToPixel(historyUv, constants.globalResolution);

    vec2 moments;
    moments.x = linearToLuma(current);
    moments.y = sqr(moments.x);

    vec4 history = vec4(current, 0.0);
    vec2 momentsHistory = moments;

    if (isPixelInBounds(historyGlobalPixel, constants.globalResolution) && false)
    {
        GBufferData historyGBufferData = GBufferData_loadAll(gbufferHistory, nearestClampSampler, historyGlobalPixel, constants.globalResolution);

        if (!GBufferData_isDisoccludedStrict(currentGBufferData, historyGBufferData))
        {
            history = texture(sampler2D(inHistory, linearClampSampler), historyUv);
            momentsHistory = imageLoad(variance, historyPixel).gb;

            history.w = min(history.w + 1.0, MAX_ACCUMULATED_FRAMES);
        }
    }

    float alpha = (history.w <= 2.0) ? 1.0 : (1.0 / (1.0 + history.w));

    vec3 result = mix(history.rgb, current, alpha);
    history.rgb = result;

    moments = mix(momentsHistory, moments, alpha);
    float newVariance = max(moments.y - sqr(moments.x), 0.0);

    imageStore(outImage, pixel, vec4(result, 1.0));
    imageStore(outHistory, pixel, history);
    imageStore(outVariance, pixel, vec4(newVariance, moments, 0.0));
}