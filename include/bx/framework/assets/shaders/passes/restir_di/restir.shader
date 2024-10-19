#ifndef RESTIR_H
#define RESTIR_H

#include "[engine]/shaders/reservoir.shader"
#include "[engine]/shaders/packing.shader"

struct ReservoirData
{
    uint triangleLightSource;
    uint blasInstance;
    vec2 hitUv;
    float p_hat;
};

struct PackedReservoirData
{
    uint triangleLightSource;
    uint blasInstance;
    uint packedHitUv;
    float p_hat;
};

ReservoirData ReservoirData_default()
{
    return ReservoirData(0, 0, vec2(0.0), 0.0);
}

bool ReservoirData_isValid(ReservoirData self)
{
    return self.p_hat != 0.0;
}

PackedReservoirData ReservoirData_toPacked(ReservoirData self)
{
    return PackedReservoirData(
        self.triangleLightSource,
        self.blasInstance,
        packHalf2x16(self.hitUv),
        self.p_hat
    );
}

ReservoirData ReservoirData_fromPacked(PackedReservoirData packed)
{
    return ReservoirData(
        packed.triangleLightSource,
        packed.blasInstance,
        unpackHalf2x16(packed.packedHitUv),
        packed.p_hat
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