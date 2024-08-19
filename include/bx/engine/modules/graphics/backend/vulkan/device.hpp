#pragma once

#include "bx/engine/core/guard.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Instance;
	class PhysicalDevice;

    class Device : NoCopy {
    public:
        Device(std::shared_ptr<Instance> instance,
            const PhysicalDevice& physicalDevice, bool debug);
        ~Device();

        void WaitIdle() const;

        VkDevice GetDevice() const;
        VmaAllocator GetAllocator() const;

    private:
        VkDevice device;
        VmaAllocator allocator;

        const std::shared_ptr<Instance> instance;
    };
}