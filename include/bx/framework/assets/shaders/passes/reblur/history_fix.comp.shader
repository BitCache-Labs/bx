#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/color_helpers.shader"
#include "[engine]/shaders/sampling.shader"

const int ANTI_FIREFLY_RADIUS = 4;
const float ANTI_FIREFLY_SCALE = 2.0;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    bool antiFirefly;
    uint _PADDING0;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D inImage;
layout (BINDING(0, 2)) uniform sampler2D mippedBlur;
layout (BINDING(0, 3), rgba32f) uniform image2D outHistory;
layout (BINDING(0, 4), rgba32f) uniform image2D outImage;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    bool disoccluded = (imageLoad(outHistory, pixel).w == 0.0);

    vec3 current = imageLoad(inImage, pixel).rgb;

    vec3 result = current;

    if (disoccluded)
    {
        vec2 uv = vec2(pixel) / vec2(constants.resolution);
        float depth = textureLod(mippedBlur, uv, 0).w * 1000.0;

        #pragma unroll
        for (int i = 3; i >= 1; i--)
        {
            vec4 mippedData = textureLod(mippedBlur, uv, i);
            float mippedDepth = mippedData.w * 1000.0;

            //if (abs(mippedDepth - depth) < 0.00002 && i > 1 && mippedDepth == depth)
            //{
            //    continue;
            //}
            //
            //if (i == 1) result = vec3(1.0, 0.0, 0.0);
            //if (i == 2) result = vec3(0.0, 1.0, 0.0);
            //if (i == 3) result = vec3(0.0, 0.0, 1.0);

            result = mippedData.rgb;
            break;
        }
    }

    if (constants.antiFirefly)
    {
        float luma = linearToLuma(result);

        float m1 = 0.0;
        float m2 = 0.0;

        #pragma unroll
        for (int y = -ANTI_FIREFLY_RADIUS; y <= ANTI_FIREFLY_RADIUS; y++)
        {
            #pragma unroll
            for (int x = -ANTI_FIREFLY_RADIUS; x <= ANTI_FIREFLY_RADIUS; x++)
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

        result *= clampedLuma / luma; // TODO: incorrect
    }

    imageStore(outImage, pixel, vec4(result, 1.0));
}