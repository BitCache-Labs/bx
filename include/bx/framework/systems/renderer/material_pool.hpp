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
		Vec3 baseColorFactor;
		u32 _PADDING0;
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

private:
	BufferHandle materialDescriptorsBuffer = BufferHandle::null;

	u32 materialDescriptorCount = 0;

	std::unordered_map<ResourceHandle, u32> materialDescriptorIndices;
	List<MaterialDescriptor> materialDescriptors;

	MaterialDescriptor AllocateMaterial(const Material& material);
};