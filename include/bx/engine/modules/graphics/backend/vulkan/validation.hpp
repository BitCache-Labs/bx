#pragma once

#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;

	static const List<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

    void CheckValidationLayerSupport();

    VkBool32 VKAPI_PTR VulkanDebugCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location, int32_t msgCode,
        const char* pLayerPrefix, const char* pMsg, void* pUserData);

    class DebugNames
    {
    public:
        static void Set(const Device& device, VkObjectType type, uint64_t handle,
            const String& name);
    };
}