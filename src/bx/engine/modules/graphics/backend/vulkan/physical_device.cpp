#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/extensions.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/instance.hpp"

namespace Vk
{
    bool IsDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        bool extensionsSupported = CheckDeviceExtensionSupport(device);

        return  // deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            extensionsSupported;
    }

    std::pair<VkPhysicalDevice, bool> PickPhysicalDevice(VkInstance instance,
        VkSurfaceKHR surface) {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        BX_ASSERT(deviceCount > 0, "No vulkan compatible device found.");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        VkPhysicalDevice fallbackPhysicalDevice = VK_NULL_HANDLE;
        bool raytracingSuitable = true;
        for (auto& device : devices) {
            if (IsDeviceSuitable(device)) {
                if (CheckDeviceRaytracingExtensionSupport(device)) {
                    physicalDevice = device;
                    break;
                }
                else {
                    fallbackPhysicalDevice = device;
                }
            }
        }
        if (physicalDevice == VK_NULL_HANDLE) {
            raytracingSuitable = false;
            physicalDevice = fallbackPhysicalDevice;
        }

        BX_ASSERT(physicalDevice != VK_NULL_HANDLE, "No vulkan compatible device found.");

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        BX_LOGI("Using Device: {}", deviceProperties.deviceName);

        return std::make_pair(physicalDevice, raytracingSuitable);
    }

    PhysicalDevice::PhysicalDevice(const Instance& instance) {
        auto pickedDevice = PickPhysicalDevice(instance.GetInstance(), instance.GetSurface());
        this->physicalDevice = pickedDevice.first;
        this->rayTracingSuitable = pickedDevice.second;

        if (this->rayTracingSuitable) {
            VkPhysicalDeviceAccelerationStructurePropertiesKHR accelProperties{};
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties{};
            accelProperties.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
            pipelineProperties.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
            pipelineProperties.pNext = &accelProperties;

            VkPhysicalDeviceProperties2 props = {};
            props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props.pNext = &pipelineProperties;
            vkGetPhysicalDeviceProperties2(this->physicalDevice, &props);
            this->rayTracingProperties = RTProperties{ accelProperties, pipelineProperties };
        }
        else {
            this->rayTracingProperties = RTProperties{};
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount,
            queueFamilies.data());

        this->graphicsFamily = UINT_MAX;
        this->computeFamily = UINT_MAX;
        this->presentFamily = UINT_MAX;
        for (size_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                this->graphicsFamily = static_cast<uint32_t>(i);
            }

            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                this->computeFamily = static_cast<uint32_t>(i);
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(this->physicalDevice, static_cast<uint32_t>(i),
                instance.GetSurface(), &presentSupport);
            if (presentSupport) {
                this->presentFamily = static_cast<uint32_t>(i);
            }

            if (this->graphicsFamily != UINT_MAX && this->computeFamily != UINT_MAX &&
                this->presentFamily != UINT_MAX) {
                return;
            }
        }

        BX_LOGE("Missing queue families!");
    }

    VkPhysicalDevice PhysicalDevice::GetPhysicalDevice() const {
        return this->physicalDevice;
    }

    bool PhysicalDevice::RayTracingSuitable() const {
        return this->rayTracingSuitable;
    }

    const RTProperties& PhysicalDevice::RayTracingProperties() const {
        return this->rayTracingProperties;
    }

    uint32_t PhysicalDevice::GraphicsFamily() const {
        return this->graphicsFamily;
    }

    uint32_t PhysicalDevice::ComputeFamily() const {
        return this->computeFamily;
    }

    uint32_t PhysicalDevice::PresentFamily() const {
        return this->presentFamily;
    }

    uint32_t PhysicalDevice::FindMemoryType(uint32_t typeFilter,
        VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        BX_LOGE("Failed to find suitable memory type.");
        return 0;
    }
}