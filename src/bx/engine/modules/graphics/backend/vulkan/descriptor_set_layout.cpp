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

        for (auto& binding : bindings)
        {
            descriptorTypes[binding.binding] = binding.descriptorType;
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        if (this->layout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(this->device->GetDevice(), this->layout, nullptr);
    }
    
    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
        : layout(other.layout), device(other.device), descriptorTypes(other.descriptorTypes)
    {
        other.layout = VK_NULL_HANDLE;
        other.descriptorTypes.clear();
    }

    DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept
    {
        layout = other.layout;
        descriptorTypes = other.descriptorTypes;
        other.layout = VK_NULL_HANDLE;
        other.descriptorTypes.clear();
        return *this;
    }

    VkDescriptorSetLayout DescriptorSetLayout::GetLayout() const {
        return this->layout;
    }

    VkDescriptorType DescriptorSetLayout::GetDescriptorType(u32 binding) const
    {
        auto typeIter = descriptorTypes.find(binding);
        BX_ENSURE(typeIter != descriptorTypes.end());
        return typeIter->second;
    }

    const DescriptorSetLayout& DescriptorSetLayout::EmptyLayout(std::shared_ptr<Device> device)
    {
        static DescriptorSetLayout empty("Empty Layout", device, {});
        return empty;
    }
}