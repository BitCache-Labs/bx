#include "bx/engine/modules/graphics/backend/vulkan/descriptor_pool.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    DescriptorPool::DescriptorPool(const std::shared_ptr<Device> device) : device(device) {
        std::vector<VkDescriptorPoolSize> poolSizes = {
            VkDescriptorPoolSize { VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 * 64 },
            VkDescriptorPoolSize { VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 * 8 },
            VkDescriptorPoolSize { VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 * 8 },
            VkDescriptorPoolSize { VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER, 1024 * 8 },
            VkDescriptorPoolSize { VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 * 8 },
            VkDescriptorPoolSize { VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024 * 8 },
            VkDescriptorPoolSize { VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1024 * 2 }
        };

        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags =
            VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        createInfo.maxSets = 1024 * 64;
        createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        createInfo.pPoolSizes = poolSizes.data();

        VK_ASSERT(
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