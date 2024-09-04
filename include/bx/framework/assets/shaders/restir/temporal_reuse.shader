#include "[engine]/shaders/Language.shader"

#define RESTIR_BINDINGS

#include "[engine]/shaders/restir/restir.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint dispatchSize;
    uint _PADDING0;
    uint _PADDING1;
    uint _PADDING2;
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
        outRestirSamples[id] = originalSample;
        return;
    }

    Reservoir reservoir = makeReservoir();
    
    // TODO
    
    RestirSample outputSample = reservoir.outputSample;
    outputSample.x0 = originalSample.x0;
    outputSample.x1 = originalSample.x1;
    outputSample.weight = (1.0 / outputSample.unoccludedContributionWeight) * reservoir.weightSum;

    outRestirSamples[id] = outputSample;
}