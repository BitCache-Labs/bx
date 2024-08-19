#pragma once

#include "bx/engine/core/guard.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Instance;

    struct RTProperties {
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelProperties;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties;
    };

    class PhysicalDevice : NoCopy {
    public:
        PhysicalDevice(const Instance& instance);

        VkPhysicalDevice GetPhysicalDevice() const;
        bool RayTracingSuitable() const;
        const RTProperties& RayTracingProperties() const;
        uint32_t GraphicsFamily() const;
        uint32_t ComputeFamily() const;
        uint32_t PresentFamily() const;

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    private:
        VkPhysicalDevice physicalDevice;
        bool rayTracingSuitable;
        RTProperties rayTracingProperties;
        uint32_t graphicsFamily;
        uint32_t computeFamily;
        uint32_t presentFamily;
    };
}