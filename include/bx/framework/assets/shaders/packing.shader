#ifndef PACKING_H
#define PACKING_H

struct PackedRgb9e5
{
	uint data;
};

struct PackedNormalizedXyz10
{
	uint data;
};

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/PixelPacking_RGBE.hlsli
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
PackedRgb9e5 packRgb9e5(vec3 rgb)
{
    float max_val = uintBitsToFloat(0x477F8000u);
    float min_val = uintBitsToFloat(0x37800000u);
    vec3 clamped_rgb = clamp(rgb, vec3(0.0f), vec3(max_val));
    float max_channel = max(max(min_val, clamped_rgb.r), max(clamped_rgb.g, clamped_rgb.b));
    float bias = uintBitsToFloat((floatBitsToUint(max_channel) + 0x07804000u) & 0x7F800000u);
    rgb = clamped_rgb + bias;
    uvec3 rgbui = uvec3(
        floatBitsToUint(rgb.x),
        floatBitsToUint(rgb.y),
        floatBitsToUint(rgb.z)
    );
    uint e = (floatBitsToUint(bias) << 4u) + 0x10000000u;
    uint data = e | rgbui.b << 18 | rgbui.g << 9 | (rgbui.r & 0x1FFu);
    return PackedRgb9e5(data);
}

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/PixelPacking_RGBE.hlsli
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
vec3 unpackRgb9e5(PackedRgb9e5 packed)
{
    vec3 rgb = vec3(uvec3(packed.data, packed.data >> 9, packed.data >> 18) & uvec3(0x1FFu));
    return ldexp(rgb, ivec3(packed.data >> 27) - 24);
}

vec3 unpackRgb9e5FromFloatBits(float data)
{
    return unpackRgb9e5(PackedRgb9e5(floatBitsToUint(data)));
}

// Inspired by https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec2 dirOctQuadEncode(vec3 dir)
{
    vec2 ret_val = dir.xy / (abs(dir.x) + abs(dir.y) + abs(dir.z));
    if (dir.z < 0.0f) {
        vec2 signs = vec2(ret_val.x >= 0.0f ? 1.0f : -1.0f, ret_val.y >= 0.0f ? 1.0f : -1.0f);
        ret_val = (1.0 - abs(ret_val.yx)) * signs;
    }
    return ret_val * 0.5 + 0.5;
}

// Inspired by https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 dirOctQuadDecode(vec2 encoded_in)
{
    vec2 encoded = encoded_in * 2.0 - 1.0;
    vec3 n = vec3(encoded.x, encoded.y, 1.0 - abs(encoded.x) - abs(encoded.y));
    float t = clamp(-n.z, 0.0, 1.0);
    vec2 added = vec2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
    n.x += added.x;
    n.y += added.y;
    return normalize(n);
}

uint pack30OctEncodedDir(vec2 oct_encoded_dir, uint offset)
{
    return ((uint(round(oct_encoded_dir.y * float(0x7fff))) << 15) |
            uint(round(oct_encoded_dir.x * float(0x7fff))))
           << offset;
}

vec2 unpack30OctEncodedDir(uint packed, uint offset)
{
    return vec2(float((packed >> offset) & 0x7fff) / float(0x7fff),
                  float((packed >> (offset + 15)) & 0x7fff) / float(0x7fff));
}

PackedNormalizedXyz10 packNormalizedXyz10(vec3 data, uint offset)
{
    vec2 oct_encoded_dir = dirOctQuadEncode(data);
    return PackedNormalizedXyz10(pack30OctEncodedDir(oct_encoded_dir, offset));
}

vec3 unpackNormalizedXyz10(PackedNormalizedXyz10 packed, uint offset)
{
    return dirOctQuadDecode(unpack30OctEncodedDir(packed.data, offset));
}

uvec4 unpack4xU8(uint data)
{
    return uvec4(
        (data & 0x000000ff),
        (data & 0x0000ff00) >> 8,
        (data & 0x00ff0000) >> 16,
        (data & 0xff000000) >> 24
    );
}

#endif // PACKING_H