#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/pfn.hpp"

namespace Vk
{
    void CheckValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            BX_ASSERT(layerFound, "Validation layer '{}' is not supported.", layerName);
        }
    }

    VkBool32 VulkanDebugCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location, int32_t msgCode,
        const char* pLayerPrefix, const char* pMsg, void* pUserData) {
        if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            BX_LOGE("[{}] {}", pLayerPrefix, pMsg);
            abort();
        }
        else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
            BX_LOGW("[{}] {}", pLayerPrefix, pMsg);
        else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
            BX_LOGI("[{}] {}", pLayerPrefix, pMsg);
        else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
            BX_LOGW("[{}] {}", pLayerPrefix, pMsg);
        else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
            BX_LOGI("[{}] {}", pLayerPrefix, pMsg);
        return 0;
    }

    void DebugNames::Set(const Device& device, VkObjectType type, uint64_t handle,
        const String& name) {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = type;
        nameInfo.objectHandle = handle;
        nameInfo.pObjectName = name.c_str();

        Pfn::vkSetDebugUtilsObjectNameEXT(device.GetDevice(), &nameInfo);
    }
}