#include "[engine]/shaders/Language.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/restir/restir.shader"

#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"

const uint NUM_SPATIAL_SAMPLES = 5;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint dispatchSize;
    uint seed;
    uint width;
    uint pixelRadius;
} constants;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.dispatchSize) return;

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));
    
    RestirSample originalSample = restirSamples[id];

    if (!isRestirSampleValid(originalSample)) // TODO: if we keep this optimize code path with final write
    {
    Reservoir reservoir = makeReservoir();
    
    #pragma unroll
    for (uint i = 0; i < NUM_SPATIAL_SAMPLES; i++)
    {
        vec2 p = getUniformDiskSample(randomUniformFloat2(rngState));
        ivec2 offset = ivec2(p * constants.pixelRadius);
        uint flatOffset = offset.y * constants.width + offset.x;
        RestirSample restirSample = restirSamples[id + flatOffset];
    
        if (isRestirSampleValid(restirSample))
        {
            float weight = (1.0 / NUM_SPATIAL_SAMPLES) * restirSample.unoccludedContributionWeight * restirSample.weight;
            updateReservoir(reservoir, rngState, restirSample, weight);
        }
    }
    
    RestirSample outputSample = reservoir.outputSample;
    outputSample.x1 = originalSample.x1;
    outputSample.weight = (1.0 / outputSample.unoccludedContributionWeight) * reservoir.weightSum;

    outRestirSamples[id] = outputSample;
}