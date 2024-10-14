#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/math.hpp"
#include "bx/engine/core/resource.hpp"

#include "bx/engine/modules/graphics.hpp"

class Material;

class MaterialPool : NoCopy
{
public:
	struct MaterialDescriptor
	{
		Vec3 baseColorFactor = Vec3(1.0, 1.0, 1.0);
		u32 baseColorTexture = UINT32_MAX;
		Vec3 emissiveFactor = Vec3(0.0, 0.0, 0.0);
		b32 isMirror = false;
	};

public:
	MaterialPool();
	~MaterialPool();

	u32 SubmitInstance(const Material& material, ResourceHandle resourceHandle);
	void Submit();

	BindGroupHandle CreateBindGroup(ComputePipelineHandle pipeline) const;

	static BindGroupLayoutDescriptor GetBindGroupLayout();
	constexpr static u32 BIND_GROUP_SET = 2;

	constexpr static u32 MAX_MATERIALS = 1024 * 32;
	constexpr static u32 MAX_TEXTURES = 1024;

private:
	BufferHandle materialDescriptorsBuffer = BufferHandle::null;
	TextureHandle textures[MAX_TEXTURES];
	SamplerHandle sampler;

	u32 materialDescriptorCount = 0;

	std::unordered_map<ResourceHandle, u32> materialDescriptorIndices;
	List<MaterialDescriptor> materialDescriptors;

	u32 AvailableTextureIdx() const;
	MaterialDescriptor UpdateMaterial(const Material& material, u32 descriptorIdx);
};