#pragma once

#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/core/macros.hpp"

#include "vulkan_api.hpp"

#define VK_ASSERT(condition, ...) \
    do { \
        if (!(condition)) \
        { \
			BX_LOGE("Vulkan failed: {}", Log::Format(__VA_ARGS__)); \
            std::abort(); \
        } \
    } while (false)

#define VK_ENSURE(condition) VK_ASSERT(condition, #condition)

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