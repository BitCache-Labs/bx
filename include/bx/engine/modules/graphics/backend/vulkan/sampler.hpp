#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;
    class PhysicalDevice;

    struct SamplerInfo {
        VkFilter filterMode = VK_FILTER_LINEAR;
    };

    class Sampler : NoCopy {
    public:
        Sampler(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, const SamplerInfo& info);
        ~Sampler();

        VkSampler GetSampler() const;

    private:
        VkSampler sampler;

        const std::shared_ptr<Device> device;
    };
}