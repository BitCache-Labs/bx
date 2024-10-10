#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/color_helpers.shader"

const float LUMA_THRESHOLD = 0.1;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint sampleCount;
    bool reducedBias;
    uint seed;
    float intensity;
    float depthOffset;
    float radius;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D image;
layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D ambientEmissiveBaseColor;

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

    vec3 emissive = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(imageLoad(ambientEmissiveBaseColor, pixel).y)));
    float luma = linearToLuma(emissive);
    if (luma >= LUMA_THRESHOLD)
    {
        return;
    }

    vec4 normalAndDepth = getPixelNormalAndDepth(pixel);
    vec3 color = imageLoad(image, pixel).rgb;
    
    float screenRadius = constants.resolution.x / 130.0;
    float radius = screenRadius * constants.radius;
    float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed) * 0.5;
    ivec2 pixelSeed = pixel;
    uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed));
    float samplingAngleOffset = angleSeed * (1.0 / float(0xffffffffU)) * TWO_PI;

    float occlusion = 0.0;
    float sampleCount = 0.0;

    #pragma unroll
    for (uint i = 0; i < 64; i++)
    {
        if (i >= constants.sampleCount)
        {
            break;
        }

        float angle = float(i) * GOLDEN_ANGLE + samplingAngleOffset;
        float currentRadius = pow(float(i) / constants.sampleCount, 0.5) * radius + samplingRadiusOffset;
    
        ivec2 offset = ivec2(currentRadius * vec2(cos(angle), sin(angle)));
        ivec2 samplePixel = pixel + offset;
        samplePixel = mirrorSample(samplePixel, ivec2(constants.resolution));

        bool validSample = true;
        if (constants.reducedBias)
        {
            vec3 sampleEmissive = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(imageLoad(ambientEmissiveBaseColor, samplePixel).y)));
            float sampleLuma = linearToLuma(sampleEmissive);

            validSample = (sampleLuma < LUMA_THRESHOLD);
        }

        if (validSample)
        {
            vec4 sampleNormalAndDepth = getPixelNormalAndDepth(samplePixel);

            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(normalAndDepth.w - sampleNormalAndDepth.w));
            occlusion += (normalAndDepth.w >= sampleNormalAndDepth.w + constants.depthOffset ? 1.0 : 0.0) * rangeCheck;
            sampleCount += 1.0;
        }
    }

    occlusion = 1.0 - (occlusion / sampleCount);
    occlusion *= constants.intensity;
    color *= mix(occlusion, 1.0, luma);

    imageStore(image, pixel, vec4(color, 1.0));
}