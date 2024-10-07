#ifndef MATERIAL_H
#define MATERIAL_H

#include "[engine]/shaders/ray_tracing/material/layered_lobe.shader"

#define MAX_MATERIAL_TEXTURES 1024

struct MaterialDescriptor
{
	vec3 baseColorFactor;
	uint baseColorTexture;
    vec3 emissiveFactor;
    bool isMirror;
};

struct SampledMaterial
{
	vec3 baseColorFactor;
    vec3 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float ior;
    float transmissionFactor;
};

#ifdef MATERIAL_BINDINGS

layout(BINDING(2, 0), std430) readonly buffer _MaterialDescriptors
{
    MaterialDescriptor materialDescriptors[];
};

layout(BINDING(2, 1)) uniform sampler2D materialTextures[MAX_MATERIAL_TEXTURES];

SampledMaterial sampleMaterial(MaterialDescriptor materialDescriptor, vec2 uv)
{
	SampledMaterial sampledMaterial;

	sampledMaterial.baseColorFactor = materialDescriptor.baseColorFactor;
	if (materialDescriptor.baseColorTexture != U32_MAX)
	{
		sampledMaterial.baseColorFactor *= texture(materialTextures[materialDescriptor.baseColorTexture], uv).rgb;
	}
    sampledMaterial.emissiveFactor = materialDescriptor.emissiveFactor;

    sampledMaterial.metallicFactor = 0.0;
    sampledMaterial.roughnessFactor = 1.0;
    sampledMaterial.ior = 1.5;
    sampledMaterial.transmissionFactor = 0.0;

	return sampledMaterial;
}

#endif // MATERIAL_BINDINGS

LayeredLobe layeredLobeFromMaterial(SampledMaterial sampledMaterial)
{
    float metallic = sampledMaterial.metallicFactor;
    float roughness = sampledMaterial.roughnessFactor;
    bool transmission = sampledMaterial.transmissionFactor == 1.0;
    float ior = sampledMaterial.ior;
    vec3 baseColor = sampledMaterial.baseColorFactor;

    DiffuseLobe diffuse_lobe = DiffuseLobe(vec3(-1.0));
    TransmissionLobe transmission_lobe = TransmissionLobe(0.0);
    if (transmission)
    {
        transmission_lobe.ior = ior;
    }
    else
    {
        diffuse_lobe.albedo = max(0.0, 1.0 - metallic) * baseColor;
    }

    SpecularLobe specular_lobe;
    specular_lobe.albedo = mix(vec3(0.04), baseColor, metallic);
    specular_lobe.roughnessFactor = roughness;
    specular_lobe.metallicFactor = metallic;
    specular_lobe.ior = ior;
    specular_lobe.thickness = 0.1; // TODO: make this controllable from pbr pipeline

    LayeredLobe layered_lobe;
    layered_lobe.diffuseLobe = diffuse_lobe;
    layered_lobe.specularLobe = specular_lobe;
    layered_lobe.transmissionLobe = transmission_lobe;
    return layered_lobe;
}

// TODO:
// - fix the diffuse specular energy conservation
// - allow diffuse specular to use transmittance when roughness == 1.0 (just don't always reflect)
// - specialized conductor fresnel equation
// - allow transmittance with roughness < 1.0

#endif // MATERIAL_H