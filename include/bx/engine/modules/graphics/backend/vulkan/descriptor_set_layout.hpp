#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/list.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;

    class DescriptorSetLayout : NoCopy {
    public:
        DescriptorSetLayout(const String& name, const std::shared_ptr<Device> device,
            const List<VkDescriptorSetLayoutBinding>& bindings);
        ~DescriptorSetLayout();

        VkDescriptorSetLayout GetLayout() const;

    private:
        VkDescriptorSetLayout layout;

        const std::shared_ptr<Device> device;
    };
}