#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/descriptor_pool.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set_layout.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/buffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/sampler.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    DescriptorSet::DescriptorSet(const String& name, const std::shared_ptr<Device> device,
        const std::shared_ptr<DescriptorPool> pool,
        const std::shared_ptr<DescriptorSetLayout> layout)
        : device(device), pool(pool), layout(layout) {
        VkDescriptorSetLayout vkLayouts[] = { layout->GetLayout() };

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = pool->GetPool();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = vkLayouts;

        BX_ASSERT(!vkAllocateDescriptorSets(device->GetDevice(), &allocateInfo, &this->descriptorSet),
            "Failed to allocate descriptor set.");
        DebugNames::Set(*device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET,
            reinterpret_cast<uint64_t>(this->descriptorSet), name);
    }

    DescriptorSet::~DescriptorSet() {
        vkFreeDescriptorSets(this->device->GetDevice(), this->pool->GetPool(), 1,
            &this->descriptorSet);
    }

    void DescriptorSet::SetBuffer(uint32_t binding, VkDescriptorType type,
        std::shared_ptr<Buffer> buffer) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = buffer->Size();

        VkWriteDescriptorSet writeInfo{};
        writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeInfo.dstSet = this->descriptorSet;
        writeInfo.dstBinding = binding;
        writeInfo.descriptorCount = 1;
        writeInfo.descriptorType = type;
        writeInfo.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(this->device->GetDevice(), 1, &writeInfo, 0, nullptr);
    }

    void DescriptorSet::SetImage(uint32_t binding, VkDescriptorType type,
        std::shared_ptr<Image> image, std::shared_ptr<Sampler> sampler) {
        VkDescriptorImageInfo imageInfo{};
        if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
            // TODO: this assumption may break, storage images are allowed to be read in a fragment shader, query resource state tracker for accurate states
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        }
        else {
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        imageInfo.imageView = image->GetImageView();
        if (sampler)
            imageInfo.sampler = sampler->GetSampler();

        VkWriteDescriptorSet writeInfo{};
        writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeInfo.dstSet = this->descriptorSet;
        writeInfo.dstBinding = binding;
        writeInfo.descriptorCount = 1;
        writeInfo.descriptorType = type;
        writeInfo.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(this->device->GetDevice(), 1, &writeInfo, 0, nullptr);
    }

    VkDescriptorSet DescriptorSet::GetDescriptorSet() const {
        return this->descriptorSet;
    }
}