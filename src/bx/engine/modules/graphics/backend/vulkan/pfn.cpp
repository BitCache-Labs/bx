#include "bx/engine/modules/graphics/backend/vulkan/pfn.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    PFN_vkGetBufferDeviceAddress Pfn::vkGetBufferDeviceAddress = VK_NULL_HANDLE;

    PFN_vkSetDebugUtilsObjectNameEXT Pfn::vkSetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;
    PFN_vkCreateDebugReportCallbackEXT Pfn::vkCreateDebugReportCallbackEXT = VK_NULL_HANDLE;
    PFN_vkDestroyDebugReportCallbackEXT Pfn::vkDestroyDebugReportCallbackEXT = VK_NULL_HANDLE;

    PFN_vkCreateAccelerationStructureKHR Pfn::vkCreateAccelerationStructureKHR = VK_NULL_HANDLE;
    PFN_vkDestroyAccelerationStructureKHR Pfn::vkDestroyAccelerationStructureKHR = VK_NULL_HANDLE;
    PFN_vkGetAccelerationStructureBuildSizesKHR Pfn::vkGetAccelerationStructureBuildSizesKHR =
        VK_NULL_HANDLE;
    PFN_vkCmdBuildAccelerationStructuresKHR Pfn::vkCmdBuildAccelerationStructuresKHR =
        VK_NULL_HANDLE;

    void Pfn::Load(VkInstance instance) {
        Pfn::vkGetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddress)vkGetInstanceProcAddr(
            instance, "vkGetBufferDeviceAddress");
        VK_ENSURE(Pfn::vkGetBufferDeviceAddress);

        Pfn::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
            instance, "vkSetDebugUtilsObjectNameEXT");
        Pfn::vkCreateDebugReportCallbackEXT =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                instance, "vkCreateDebugReportCallbackEXT");
        Pfn::vkDestroyDebugReportCallbackEXT =
            (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                instance, "vkDestroyDebugReportCallbackEXT");
        VK_ENSURE(Pfn::vkSetDebugUtilsObjectNameEXT);
        VK_ENSURE(Pfn::vkCreateDebugReportCallbackEXT);
        VK_ENSURE(Pfn::vkDestroyDebugReportCallbackEXT);

        Pfn::vkCreateAccelerationStructureKHR =
            (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(
                instance, "vkCreateAccelerationStructureKHR");
        Pfn::vkDestroyAccelerationStructureKHR =
            (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(
                instance, "vkDestroyAccelerationStructureKHR");
        Pfn::vkGetAccelerationStructureBuildSizesKHR =
            (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(
                instance, "vkGetAccelerationStructureBuildSizesKHR");
        Pfn::vkCmdBuildAccelerationStructuresKHR =
            (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(
                instance, "vkCmdBuildAccelerationStructuresKHR");
        VK_ENSURE(Pfn::vkCreateAccelerationStructureKHR);
        VK_ENSURE(Pfn::vkDestroyAccelerationStructureKHR);
        VK_ENSURE(Pfn::vkGetAccelerationStructureBuildSizesKHR);
        VK_ENSURE(Pfn::vkCmdBuildAccelerationStructuresKHR);
    }
}