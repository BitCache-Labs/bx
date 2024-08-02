#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/extensions.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/instance.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Vk
{
    Device::Device(std::shared_ptr<Instance> instance,
        const PhysicalDevice& physicalDevice, bool debug)
        : instance(instance) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = physicalDevice.GraphicsFamily();
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.fillModeNonSolid = VK_TRUE;
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
        accelerationStructureFeatures.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        accelerationStructureFeatures.accelerationStructure = true;

        VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures{};
        bufferDeviceAddressFeatures.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
        bufferDeviceAddressFeatures.bufferDeviceAddress = true;
        bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = true;

        if (physicalDevice.RayTracingSuitable()) {
            bufferDeviceAddressFeatures.pNext = &accelerationStructureFeatures;
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        if (physicalDevice.RayTracingSuitable()) {
            createInfo.pNext = &bufferDeviceAddressFeatures;
        }
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (debug) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        BX_ASSERT(!vkCreateDevice(physicalDevice.GetPhysicalDevice(), &createInfo, nullptr,
            &this->device),
            "Failed to create logical device.");
        VmaAllocatorCreateInfo vmaCreateInfo{};
        vmaCreateInfo.vulkanApiVersion = VULKAN_VERSION;
        vmaCreateInfo.instance = instance->GetInstance();
        vmaCreateInfo.physicalDevice = physicalDevice.GetPhysicalDevice();
        vmaCreateInfo.device = this->device;
        // NOTE: these will need to be loaded on some platforms, just like the core vulkan functions
        vmaCreateInfo.pVulkanFunctions = nullptr;
        if (physicalDevice.RayTracingSuitable()) {
            vmaCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        BX_ASSERT(!vmaCreateAllocator(&vmaCreateInfo, &this->allocator),
            "Failed to create allocator.");
    }

    Device::~Device() {
        this->WaitIdle();
        vmaDestroyAllocator(this->allocator);
        vkDestroyDevice(this->device, nullptr);
    }

    void Device::WaitIdle() const {
        vkDeviceWaitIdle(this->device);
    }

    VkDevice Device::GetDevice() const {
        return this->device;
    }

    VmaAllocator Device::GetAllocator() const {
        return this->allocator;
    }
}