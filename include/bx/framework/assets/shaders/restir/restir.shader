#ifndef RESTIR_H
#define RESTIR_H

#include "[engine]/shaders/reservoir.shader"
#include "[engine]/shaders/packing.shader"

const uint JACOBIAN_CUTOFF = 8;
const float RESERVOIR_M_CLAMP = 20.0;

struct ReservoirData
{
    vec3 sampleDirection;
    float hitT;
    uint triangleLightSource;
    uint blasInstance;
    vec2 hitUv;
    float unoccludedContributionWeight;
};

struct PackedReservoirData
{
    PackedNormalizedXyz10 packedSampleDirection;
    float hitT;
    uint triangleLightSource;
    uint blasInstance;
    uint packedHitUv;
    float unoccludedContributionWeight;
    uint _PADDING0;
    uint _PADDING1;
};

PackedReservoirData ReservoirData_toPacked(ReservoirData self)
{
    return PackedReservoirData(
        packNormalizedXyz10(self.sampleDirection, 0),
        self.hitT,
        self.triangleLightSource,
        self.blasInstance,
        packHalf2x16(self.hitUv),
        self.unoccludedContributionWeight,
        0, 0
    );
}

ReservoirData ReservoirData_fromPacked(PackedReservoirData packed)
{
    return ReservoirData(
        unpackNormalizedXyz10(packed.packedSampleDirection, 0),
        packed.hitT,
        packed.triangleLightSource,
        packed.blasInstance,
        unpackHalf2x16(packed.packedHitUv),
        packed.unoccludedContributionWeight
    );
}

#ifdef RESTIR_BINDINGS

layout (BINDING(4, 0), std430) buffer _RestirReservoirs
{
    PackedReservoir restirReservoirs[];
};

layout (BINDING(4, 1), std430) buffer _OutRestirReservoirs
{
    PackedReservoir outRestirReservoirs[];
};

layout (BINDING(4, 2), std430) buffer _RestirReservoirsHistory
{
    PackedReservoir restirReservoirsHistory[];
};

layout (BINDING(4, 3), std430) buffer _RestirReservoirData
{
    PackedReservoirData restirReservoirData[];
};

layout (BINDING(4, 4), std430) buffer _OutRestirReservoirData
{
    PackedReservoirData outRestirReservoirData[];
};

layout (BINDING(4, 5), std430) buffer _RestirReservoirDataHistory
{
    PackedReservoirData restirReservoirDataHistory[];
};

#endif // RESTIR_BINDINGS

#endif // RESTIR_H