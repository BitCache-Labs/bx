#include "[engine]/shaders/Language.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/restir/restir.shader"

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

    RestirSample currentSample = restirSamples[id];
    RestirSample previousSample = restirSamplesHistory[id];

    if (!isRestirSampleValid(currentSample))
    {
        restirSamplesHistory[id] = currentSample;
        return;
    }

    Reservoir reservoir = makeReservoir();
    
    float weight = (1.0 / 2.0) * currentSample.unoccludedContributionWeight * currentSample.weight;
    updateReservoir(reservoir, rngState, currentSample, weight);
    
    weight = (1.0 / 2.0) * previousSample.unoccludedContributionWeight * previousSample.weight;
    updateReservoir(reservoir, rngState, previousSample, weight);
    
    RestirSample outputSample = reservoir.outputSample;
    outputSample.x0 = currentSample.x0;
    outputSample.x1 = currentSample.x1;
    outputSample.weight = (1.0 / outputSample.unoccludedContributionWeight) * reservoir.weightSum;
    
    restirSamples[id] = outputSample;
    restirSamplesHistory[id] = outputSample;
}