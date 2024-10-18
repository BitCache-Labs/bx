#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/math.shader"
#include "[engine]/shaders/color_helpers.shader"
#include "[engine]/shaders/sampling.shader"

const int ANTI_FIREFLY_RADIUS = 4;
const float ANTI_FIREFLY_SCALE = 2.0;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1)) uniform texture2D image;
layout (BINDING(0, 2), rgba32f) uniform image2D outImage;

layout (BINDING(0, 3)) uniform sampler linearClampSampler;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec3 color = texture(sampler2D(image, linearClampSampler), pixelToUv(pixel, constants.resolution)).rgb;
    float luma = linearToLuma(color);

    if (luma != 0.0)
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

                float sampleLuma = linearToLuma(texture(sampler2D(image, linearClampSampler), pixelToUv(samplePixel, constants.resolution)).rgb);
                m1 += sampleLuma;
                m2 += sqr(sampleLuma);
            }
        }

        float invNorm = 1.0 / ((ANTI_FIREFLY_RADIUS * 2 + 1) * (ANTI_FIREFLY_RADIUS * 2 + 1) - 3 * 3);
        m1 *= invNorm;
        m2 *= invNorm;

        float sigma = stdDev(m1, m2) * ANTI_FIREFLY_SCALE;
        float clampedLuma = clamp(luma, m1 - sigma, m1 + sigma);

        vec3 colorLod = textureLod(sampler2D(image, linearClampSampler), pixelToUv(pixel, constants.resolution), 4.0).rgb;
        float lodLuma = linearToLuma(colorLod);
        clampedLuma = min(clampedLuma, lodLuma);

        float lumaFactor = clampedLuma / luma;
        
        color *= lumaFactor; // TODO: incorrect
    }

    imageStore(outImage, pixel, vec4(color, 1.0));
}