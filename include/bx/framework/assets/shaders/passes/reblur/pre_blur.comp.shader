#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D inImage;
layout (BINDING(0, 2), rgba32f) uniform image2D gbuffer;
layout (BINDING(0, 3), rgba32f) uniform image2D outImage;

float KERNEL[9] = float[](
    1.0, 2.0, 1.0,
    2.0, 4.0, 2.0,
    1.0, 2.0, 1.0
);

vec4 getPixelNormalAndDepth(ivec2 pixel, out uint blasInstance)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    blasInstance = floatBitsToUint(gbufferData.a);
    return vec4(normal, (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r);
}

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    uint currentBlasInstance;
    vec4 currentNormalAndDepth = getPixelNormalAndDepth(pixel, currentBlasInstance);

    vec3 result = vec3(0.0);
    float sampleCount = 0.0;

    #pragma unroll
    for (uint y = 0; y < 3; y++)
    {
        #pragma unroll
        for (uint x = 0; x < 3; x++)
        {
            ivec2 samplePixel = pixel + ivec2(x - 1, y - 1) * 2;
            samplePixel = mirrorSample(samplePixel, ivec2(constants.resolution));

            uint sampleBlasInstance;
            vec4 sampleNormalAndDepth = getPixelNormalAndDepth(samplePixel, sampleBlasInstance);

            bool validDepth = abs(currentNormalAndDepth.w - sampleNormalAndDepth.w) < 0.6 && sampleNormalAndDepth.w != 0.0;
            bool validNormals = dot(currentNormalAndDepth.xyz, sampleNormalAndDepth.xyz) >= 0.86;
            bool validBlasInstance = currentBlasInstance == sampleBlasInstance;

            if (validDepth && validNormals && validBlasInstance)
            {
                float weight = KERNEL[y * 3 + x];
                result += imageLoad(inImage, samplePixel).rgb * weight;
                sampleCount += weight;
            }
        }
    }

    result /= sampleCount;

    imageStore(outImage, pixel, vec4(result, currentNormalAndDepth.w));
}