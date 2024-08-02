#include "bx/engine/modules/graphics/backend/vulkan/descriptor_pool.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"

namespace Vk
{
    DescriptorPool::DescriptorPool(const std::shared_ptr<Device> device) : device(device) {
        // TODO: increase this stuff based on some device limits
        std::vector<VkDescriptorPoolSize> poolSizes = {
            VkDescriptorPoolSize{VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64} };

        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags =
            VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        createInfo.maxSets = 512;
        createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        createInfo.pPoolSizes = poolSizes.data();

        BX_ASSERT(
            !vkCreateDescriptorPool(device->GetDevice(), &createInfo, nullptr, &this->pool),
            "Failed to create descriptor pool.");
    }

    DescriptorPool::~DescriptorPool() {
        vkDestroyDescriptorPool(this->device->GetDevice(), this->pool, nullptr);
    }

    VkDescriptorPool DescriptorPool::GetPool() const {
        return this->pool;
    }
}