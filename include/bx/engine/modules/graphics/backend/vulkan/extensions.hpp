#pragma once

#include "vulkan_api.hpp"

#include "bx/engine/containers/list.hpp"

namespace Vk
{
    static const List<const char*> instanceExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                                                                VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                                                                VK_KHR_SURFACE_EXTENSION_NAME };
    static const List<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME };

    static const List<const char*> deviceRaytracingExtensions = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME };

    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
    bool CheckDeviceRaytracingExtensionSupport(VkPhysicalDevice physicalDevice);

    List<const char*> PlatformInstanceExtensions();
}