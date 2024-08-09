#pragma once

#include "bx/engine/core/guard.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;

    class DescriptorPool : NoCopy {
    public:
        DescriptorPool(const std::shared_ptr<Device> device);
        ~DescriptorPool();

        VkDescriptorPool GetPool() const;

    private:
        VkDescriptorPool pool;

        const std::shared_ptr<Device> device;
    };
}