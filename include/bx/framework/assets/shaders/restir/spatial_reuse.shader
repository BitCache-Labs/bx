#include "[engine]/shaders/Language.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/restir/restir.shader"

#include "[engine]/shaders/sampling.shader"
#include "[engine]/shaders/random.shader"

const int NUM_SPATIAL_SAMPLES = 8;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint dispatchSize;
    uint seed;
    uint _PADDING1;
    uint _PADDING2;
} constants;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.dispatchSize) return;

    uint rngState = pcgHash(id ^ xorShiftU32(constants.seed));

    Reservoir reservoir;
    reservoir.weightSum = 0.0;

    #pragma unroll
    for (uint i = 0; i < NUM_SPATIAL_SAMPLES; i++)
    {
        vec2 p = getUniformDiskSample(randomUniformFloat2(rngState));
        RestirSample restirSample = restirSamples[id];

        float weight = (1.0 / NUM_SPATIAL_SAMPLES) * restirSample.unoccludedContributionWeight * restirSample.weight;
        updateReservoir(reservoir, rngState, restirSample, weight);
    }

    RestirSample outputSample = reservoir.outputSample;
    outputSample.weight = reservoir.weightSum * outputSample.weight;

    // TODO: fix race condition, will require a second outRestirSamples buffer
}