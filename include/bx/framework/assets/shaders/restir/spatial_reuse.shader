#include "[engine]/shaders/Language.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/restir/restir.shader"
#include "[engine]/shaders/restir/jacobian.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"

const uint NUM_SPATIAL_SAMPLES = 5;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint dispatchSize;
    uint seed;
    uint width;
    uint spatialIndex;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D gbuffer;

vec4 getPixelNormalAndDepth(ivec2 pixel)
{
    vec4 gbufferData = imageLoad(gbuffer, pixel);
    vec3 normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    return vec4(normal, 1.0 / gbufferData.r);
}

bool validatePixelSimilarity(vec4 normalAndDepth, vec4 otherNormalAndDepth)
{
    return true;
    bool similairNormals = (1.0 - abs(dot(normalAndDepth.xyz, otherNormalAndDepth.xyz))) < 0.2;
    bool similairDepth = abs(1.0 - (normalAndDepth.w / otherNormalAndDepth.w)) < 0.1;
    return similairNormals && similairDepth;
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.dispatchSize) return;
    ivec2 pixel = ivec2(int(id % constants.width), int(id / constants.width));
    
    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));
    
    Reservoir reservoir = restirReservoirs[id];
    RestirSample originalSample = reservoir.outputSample;
    vec4 originalNormalAndDepth = getPixelNormalAndDepth(pixel);

    if (isReservoirValid(reservoir))
    {
        float screenRadius = constants.width / 30.0;
        float radius = screenRadius; // TODO: use validity

        float samplingRadiusOffset = interleavedGradientNoiseAnimated(uvec2(pixel), constants.seed * 2 + constants.spatialIndex) * 0.5;

        ivec2 pixelSeed = (constants.spatialIndex == 0) ? (pixel >> 3) : (pixel >> 2);
        uint angleSeed = hashCombine(pixelSeed.x, hashCombine(pixelSeed.y, constants.seed * 2 + constants.spatialIndex));
        float samplingAngleOffset = angleSeed * (1.0 / float(0xffffffffU)) * TWO_PI;

        #pragma unroll
        for (uint i = 0; i < NUM_SPATIAL_SAMPLES; i++)
        {
            float angle = float(i) * GOLDEN_ANGLE + samplingAngleOffset;
            float currentRadius = pow(float(i) / NUM_SPATIAL_SAMPLES, 0.5) * radius + samplingRadiusOffset;

            ivec2 offset = ivec2(currentRadius * vec2(cos(angle), sin(angle)));
            
            ivec2 candidatePixel = pixel + offset;
            uint flatOffset = offset.y * constants.width + offset.x;
            if (candidatePixel.x >= constants.width || flatOffset >= constants.dispatchSize)
            {
                continue;
            }
            
            Reservoir candidateReservoir = restirReservoirs[id + flatOffset];
            vec4 otherNormalAndDepth = getPixelNormalAndDepth(pixel + offset);
        
            if (isReservoirValid(candidateReservoir) && validatePixelSimilarity(originalNormalAndDepth, otherNormalAndDepth))
            {
                reservoir = combineReservoirs(rngState, reservoir, candidateReservoir);
                reservoir.sampleCount = min(reservoir.sampleCount, 1024 * 64);
            }
        }
        
        reservoir.outputSample.x0 = originalSample.x0;
        reservoir.outputSample.x1 = originalSample.x1;
    }
    
    outRestirReservoirs[id] = reservoir;
    restirReservoirsHistory[id] = reservoir;
}