#pragma once

#include "vulkan_api.hpp"

namespace Vk
{
	inline bool IsDepthFormat(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	inline bool IsStencilFormat(VkFormat format)
	{
		return format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	inline VkImageLayout GetDepthStencilLayout(VkFormat format)
	{
		b8 depth = 
			format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT ||
			format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT ||
			format == VK_FORMAT_D32_SFLOAT_S8_UINT;
		b8 stencil =
			format == VK_FORMAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
			format == VK_FORMAT_D32_SFLOAT_S8_UINT;

		if (depth && stencil)
		{
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else if (depth)
		{
			return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		}
		else
		{
			return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
		}
	}
}