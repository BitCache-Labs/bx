#pragma once

#include "vulkan_api.hpp"

namespace Vk
{
	class Pfn
	{
	public:
		static void Load(VkInstance instance);

        static PFN_vkGetBufferDeviceAddress vkGetBufferDeviceAddress;

        static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
        static PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
        static PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;

        static PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
        static PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
        static PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
        static PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	};
}