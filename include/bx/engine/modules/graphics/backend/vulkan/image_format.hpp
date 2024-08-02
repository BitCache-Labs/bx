#pragma once

#include "vulkan_api.hpp"

namespace Vk
{
	inline bool IsDepthFormat(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
}