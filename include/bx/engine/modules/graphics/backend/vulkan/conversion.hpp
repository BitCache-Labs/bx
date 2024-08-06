#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"
#include "bx/engine/modules/graphics/type.hpp"

namespace Vk
{
	VkBufferUsageFlags BufferUsageFlagsToVk(BufferUsageFlags usage);
	VkImageUsageFlags TextureUsageFlagsToVk(TextureUsageFlags usage, b8 depthFormat);
	VkFormat TextureFormatToVk(TextureFormat format);
	VkImageType TextureDimensionToVk(TextureDimension dimension);
	VkShaderStageFlagBits ShaderTypeToVk(ShaderType type);
	VkDescriptorType BindingTypeToVk(BindingType type);
	VkShaderStageFlags ShaderStageFlagsToVk(ShaderStageFlags stageFlags);
	VkFormat VertexFormatToVk(const VertexFormat& format);
}