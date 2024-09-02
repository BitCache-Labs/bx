#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/hash_map.hpp"
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
        DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
        DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

        VkDescriptorSetLayout GetLayout() const;
        VkDescriptorType GetDescriptorType(u32 binding) const;

        static const DescriptorSetLayout& EmptyLayout(std::shared_ptr<Device> device);

    private:
        VkDescriptorSetLayout layout;

        HashMap<u32, VkDescriptorType> descriptorTypes;

        const std::shared_ptr<Device> device;
    };
}