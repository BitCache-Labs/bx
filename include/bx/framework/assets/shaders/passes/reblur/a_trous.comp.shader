#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"

const float PHI_COLOR = 10.0;
const float PHI_NORMAL = 128.0;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 globalResolution;
    uvec2 resolution;
    uint stepSize;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D inImage;
layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D outImage;

//layout (BINDING(0, 4)) uniform sampler linearClampSampler;

vec4 getPixelNormalAndDepth(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;
    ivec2 globalPixel = rescaleResolution(pixel, constants.resolution, constants.globalResolution);

    vec4 currentNormalAndDepth = getPixelNormalAndDepth(globalPixel);

    vec3 result = imageLoad(inImage, pixel).rgb;

    if (currentNormalAndDepth.w == 0.0)
    {
        imageStore(outImage, pixel, vec4(result, 1.0));
        return;
    }

    vec3 sum = vec3(0.0);
    vec3 cval = result;
    vec3 nval = currentNormalAndDepth.xyz;
    float cum_w = 0.0;

    #pragma unroll
    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            //if (x == 0 && y == 0)
            //{
            //    continue;
            //}

            ivec2 samplePixel = pixel + ivec2(x, y) * int(constants.stepSize);
            //vec2 sampleUv = pixelToUv(samplePixel);
            ivec2 globalSamplePixel = rescaleResolution(samplePixel, constants.resolution, constants.globalResolution);
            //vec2 globalSampleUv = pixelToUv(globalSamplePixel);

            vec4 sampleNormalAndDepth = getPixelNormalAndDepth(globalSamplePixel);

            vec3 ctmp = imageLoad(inImage, samplePixel).rgb;
            vec3 t = cval - ctmp;

            float dist2 = dot(t, t);
            float c_w = min(exp(-(dist2) / PHI_COLOR), 1.0);

            t = nval - sampleNormalAndDepth.xyz;
            dist2 = dot(t, t);
            float n_w = min(exp(-(dist2) / PHI_NORMAL), 1.0);

            float weight = c_w * n_w;
            sum += ctmp * weight;
            cum_w += weight;
        }
    }
    sum /= cum_w;

    imageStore(outImage, pixel, vec4(sum, 1.0));

    //float animatedNoise = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 3 + 1) * 0.5;
    //
    //#pragma unroll
    //for (int x = -RADIUS; x <= RADIUS; x++)
    //{
    //    #pragma unroll
    //    for (int y = -RADIUS; y <= RADIUS; y++)
    //    {
    //        if (x == 0 && y == 0)
    //        {
    //            continue;
    //        }
    //
    //        ivec2 offset = ivec2(x, y);
    //        uint flatOffset = offset.y * constants.resolution.x + offset.x;
    //        ivec2 samplePixel = pixel + offset;
    //        samplePixel = mirrorSample(samplePixel, ivec2(constants.resolution));
    //        ivec2 globalSamplePixel = rescaleResolution(samplePixel, constants.resolution, constants.globalResolution);
    //        
    //        uint sampleBlasInstance;
    //        vec4 sampleNormalAndDepth = getPixelNormalAndDepth(globalSamplePixel, sampleBlasInstance);
    //        
    //        float weight;
    //        if (sampleToCurrentSimilarity(currentNormalAndDepth, sampleNormalAndDepth, currentBlasInstance, sampleBlasInstance, weight))
    //        {
    //            float lod = (1.0 / (1.0 + frameCount)) * 2.0 + 1.0 + animatedNoise;
    //
    //            result += textureLod(sampler2D(inImage, linearClampSampler), vec2(samplePixel) / vec2(constants.resolution), lod).rgb * weight;
    //            sampleCount += weight;
    //        }
    //    }
    //}
    //
    //result /= sampleCount;
    //
    //imageStore(outImage, pixel, vec4(result, 1.0));
    //imageStore(outHistory, pixel, vec4(fixNan(result), frameCount));
}