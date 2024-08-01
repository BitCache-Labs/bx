#include "bx/engine/modules/graphics/type_validation.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/containers/list.hpp"

b8 ValidateBufferCreateInfo(const BufferCreateInfo& createInfo)
{
	if (createInfo.usageFlags == (BufferUsageFlags)0)
	{
		BX_LOGE("Invalid buffer creation info: usageFlags cannot be empty.");
		return false;
	}

	if (createInfo.size == 0)
	{
		BX_LOGE("Invalid buffer creation info: size ({}) must be larger than 0.", createInfo.size);
		return false;
	}

	return true;
}

b8 ValidateSamplerCreateInfo(const SamplerCreateInfo& createInfo)
{
	if (createInfo.lodMinClamp < 0.0)
	{
		BX_LOGE("Invalid sampler creation info: lodMinClamp ({}) cannot be negative.", createInfo.lodMinClamp);
		return false;
	}

	if (createInfo.lodMinClamp < createInfo.lodMaxClamp)
	{
		BX_LOGE("Invalid sampler creation info: lodMinClamp ({}) must be smaller than lodMaxClamp ({}).", createInfo.lodMinClamp, createInfo.lodMaxClamp);
		return false;
	}

	if ((createInfo.addressModeU == SamplerAddressMode::CLAMP_TO_BORDER
		|| createInfo.addressModeV == SamplerAddressMode::CLAMP_TO_BORDER
		|| createInfo.addressModeW == SamplerAddressMode::CLAMP_TO_BORDER)
		&& createInfo.borderColor.IsNone())
	{
		BX_LOGE("Invalid sampler creation info: borderColor must be Some when AddressMode::CLAMP_TO_BORDER is used.");
		return false;
	}

	return true;
}

b8 ValidateTextureCreateInfo(const TextureCreateInfo& createInfo)
{
	if (createInfo.mipLevelCount == 0)
	{
		BX_LOGE("Invalid texture creation info: mipLevelCount ({}) must be 1 or larger.", createInfo.mipLevelCount);
		return false;
	}

	if (createInfo.sampleCount == 0)
	{
		BX_LOGE("Invalid texture creation info: sampleCount ({}) must be 1 or larger.", createInfo.sampleCount);
		return false;
	}

	if (createInfo.usageFlags == (TextureUsageFlags)0)
	{
		BX_LOGE("Invalid texture creation info: usageFlags cannot be empty.");
		return false;
	}

	if ((createInfo.dimension != TextureDimension::D2 || createInfo.size.depthOrArrayLayers != 1) && (createInfo.usageFlags & TextureUsageFlags::RENDER_ATTACHMENT))
	{
		BX_LOGE("Invalid texture creation info: texture can only be used as render attachment if dimension = D2 and depthOrArrayLayers = 1.");
		return false;
	}

	return true;
}

b8 ValidateShaderCreateInfo(const ShaderCreateInfo& createInfo)
{
	if (createInfo.src.empty())
	{
		BX_LOGE("Invalid shader creation info: src cannot be empty.");
		return false;
	}

	return true;
}

b8 ValidateBindGroupCreateInfo(const BindGroupCreateInfo& createInfo)
{
	if (createInfo.layout == BindGroupLayoutHandle::null)
	{
		BX_LOGE("Invalid bind group creation info: layout cannot be null.");
		return false;
	}

	List<b8> usedBindings(1024);
	for (auto& entry : createInfo.entries)
	{
		if (usedBindings[entry.binding])
		{
			BX_LOGE("Invalid bind group creation info: binding {} was used at least twice.", entry.binding);
			return false;
		}
		usedBindings[entry.binding] = true;

		const BindingResource& resource = entry.resource;
		switch (resource.type)
		{
		case BindingResourceType::BUFFER:
		{
			if (resource.buffer.buffer == BufferHandle::null)
			{
				BX_LOGE("Invalid bind group creation info: binding {} contains a null buffer.", entry.binding);
				return false;
			}
			break;
		}
		case BindingResourceType::SAMPLER:
		{
			if (resource.sampler == SamplerHandle::null)
			{
				BX_LOGE("Invalid bind group creation info: binding {} contains a null sampler.", entry.binding);
				return false;
			}
			break;
		}
		case BindingResourceType::TEXTURE_VIEW:
		{
			if (resource.textureView == TextureViewHandle::null)
			{
				BX_LOGE("Invalid bind group creation info: binding {} contains a null texture view.", entry.binding);
				return false;
			}
			break;
		}
		case BindingResourceType::BUFFER_ARRAY:
		{
			if (resource.bufferArray.empty())
			{
				BX_LOGE("Invalid bind group creation info: binding {} contains an empty buffer array.", entry.binding);
				return false;
			}

			for (u32 i = 0; i < resource.bufferArray.size(); i++)
			{
				if (resource.bufferArray[i].buffer == BufferHandle::null)
				{
					BX_LOGE("Invalid bind group creation info: binding {} contains a null buffer at index {}.", entry.binding, i);
					return false;
				}
			}
			break;
		}
		case BindingResourceType::SAMPLER_ARRAY:
		{
			if (resource.samplerArray.empty())
			{
				BX_LOGE("Invalid bind group creation info: binding {} contains an empty sampler array.", entry.binding);
				return false;
			}

			for (u32 i = 0; i < resource.samplerArray.size(); i++)
			{
				if (resource.samplerArray[i] == SamplerHandle::null)
				{
					BX_LOGE("Invalid bind group creation info: binding {} contains a null sampler at index {}.", entry.binding, i);
					return false;
				}
			}
			break;
		}
		case BindingResourceType::TEXTURE_VIEW_ARRAY:
		{
			if (resource.textureViewArray.empty())
			{
				BX_LOGE("Invalid bind group creation info: binding {} contains an empty texture view array.", entry.binding);
				return false;
			}

			for (u32 i = 0; i < resource.textureViewArray.size(); i++)
			{
				if (resource.textureViewArray[i] == TextureViewHandle::null)
				{
					BX_LOGE("Invalid bind group creation info: binding {} contains a null texture view at index {}.", entry.binding, i);
					return false;
				}
			}
			break;
		}
		}
	}

	return true;
}

b8 ValidatePipelineLayoutDescriptor(const PipelineLayoutDescriptor& descriptor, const String& creationName)
{
	List<b8> usedGroups(128);
	for (auto& groupLayout : descriptor.bindGroupLayouts)
	{
		if (usedGroups[groupLayout.group])
		{
			BX_LOGE("Invalid {} creation info: group {} was used at least twice.", creationName, groupLayout.group);
			return false;
		}
		usedGroups[groupLayout.group] = true;

		List<b8> usedBindings(1024);
		for (auto& entry : groupLayout.entries)
		{
			if (usedBindings[entry.binding])
			{
				BX_LOGE("Invalid {} creation info: binding {} in group {} was used at least twice.", creationName, entry.binding, groupLayout.group);
				return false;
			}
			usedBindings[entry.binding] = true;

			if (entry.visibility == (ShaderStageFlags)0)
			{
				BX_LOGE("Invalid {} creation info: binding {} in group {} visibility cannot be empty.", creationName, entry.binding, groupLayout.group);
				return false;
			}
		}
	}

	return true;
}

b8 ValidateGraphicsPipelineCreateInfo(const GraphicsPipelineCreateInfo& createInfo)
{
	if (createInfo.vertexShader == ShaderHandle::null)
	{
		BX_LOGE("Invalid graphics pipeline creation info: vertexShader cannot be null.");
		return false;
	}

	if (createInfo.fragmentShader == ShaderHandle::null)
	{
		BX_LOGE("Invalid graphics pipeline creation info: fragmentShader cannot be null.");
		return false;
	}
	
	if (!ValidatePipelineLayoutDescriptor(createInfo.layout, "graphics pipeline"))
	{
		return false;
	}

	return true;
}

b8 ValidateComputePipelineCreateInfo(const ComputePipelineCreateInfo& createInfo)
{
	if (createInfo.shader == ShaderHandle::null)
	{
		BX_LOGE("Invalid compute pipeline creation info: shader cannot be null.");
		return false;
	}

	if (!ValidatePipelineLayoutDescriptor(createInfo.layout, "compute pipeline"))
	{
		return false;
	}

	return true;
}