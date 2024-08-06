#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set_layout.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    DescriptorSetLayout::DescriptorSetLayout(const String& name, const std::shared_ptr<Device> device,
        const List<VkDescriptorSetLayoutBinding>& bindings)
        : device(device) {
        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createInfo.pBindings = bindings.data();

        VK_ASSERT(
            !vkCreateDescriptorSetLayout(device->GetDevice(), &createInfo, nullptr, &this->layout),
            "Failed to create descriptor set layout.");
        DebugNames::Set(*device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(this->layout),
            name);
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        if (this->layout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(this->device->GetDevice(), this->layout, nullptr);
    }
    
    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
        : layout(other.layout), device(other.device)
    {
        other.layout = VK_NULL_HANDLE;
    }

    DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept
    {
        layout = other.layout;
        other.layout = VK_NULL_HANDLE;
        return *this;
    }

    VkDescriptorSetLayout DescriptorSetLayout::GetLayout() const {
        return this->layout;
    }
}