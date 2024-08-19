#pragma once

#include "bx/engine/core/guard.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
	class Instance : NoCopy
	{
	public:
		Instance(void* window, bool debug);
		~Instance();

		VkInstance GetInstance() const;
		VkSurfaceKHR GetSurface() const;

	private:
		VkSurfaceKHR CreateSurface(void* window, VkInstance instance);

		VkDebugReportCallbackEXT debugReportCallback;
		VkInstance instance;
		VkSurfaceKHR surface;
	};
}