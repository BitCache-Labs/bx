#ifndef MATERIAL_H
#define MATERIAL_H

#include "[engine]/shaders/ray_tracing/material/layered_lobe.shader"

struct MaterialDescriptor
{
	vec3 baseColorFactor;
	uint _PADDING0;
};

struct SampledMaterial
{
	vec3 baseColorFactor;
};

SampledMaterial sampleMaterial(MaterialDescriptor materialDescriptor)
{
	SampledMaterial sampledMaterial;

	sampledMaterial.baseColorFactor = materialDescriptor.baseColorFactor;

	return sampledMaterial;
}

#ifdef MATERIAL_BINDINGS

layout(BINDING(2, 0), std430) readonly buffer _MaterialDescriptors
{
    MaterialDescriptor materialDescriptors[];
};

#endif // MATERIAL_BINDINGS

#endif // MATERIAL_H