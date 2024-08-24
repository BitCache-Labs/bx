#ifndef MATERIAL_H
#define MATERIAL_H

#include "[engine]/shaders/ray_tracing/material/layered_lobe.shader"

#define MAX_MATERIAL_TEXTURES 1024

struct MaterialDescriptor
{
	vec3 baseColorFactor;
	uint baseColorTexture;
};

struct SampledMaterial
{
	vec3 baseColorFactor;
};

#ifdef MATERIAL_BINDINGS

layout(BINDING(2, 0), std430) readonly buffer _MaterialDescriptors
{
    MaterialDescriptor materialDescriptors[];
};

layout(BINDING(2, 1)) uniform sampler2D materialTextures[MAX_MATERIAL_TEXTURES];

#endif // MATERIAL_BINDINGS

SampledMaterial sampleMaterial(MaterialDescriptor materialDescriptor, vec2 uv)
{
	SampledMaterial sampledMaterial;

	sampledMaterial.baseColorFactor = materialDescriptor.baseColorFactor;
	if (materialDescriptor.baseColorTexture != U32_MAX)
	{
		sampledMaterial.baseColorFactor *= texture(materialTextures[materialDescriptor.baseColorTexture], uv).rgb;
	}

	return sampledMaterial;
}

#endif // MATERIAL_H