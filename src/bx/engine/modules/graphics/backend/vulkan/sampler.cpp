#include "bx/engine/modules/graphics/backend/vulkan/sampler.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    Sampler::Sampler(const String& name, std::shared_ptr<Device> device,
        const PhysicalDevice& physicalDevice, const SamplerInfo& info)
        : device(device) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice.GetPhysicalDevice(), &properties);

        VkSamplerCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.magFilter = info.filterMode;
        createInfo.minFilter = info.filterMode;
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.anisotropyEnable = VK_TRUE;
        createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        createInfo.unnormalizedCoordinates = VK_FALSE;
        createInfo.compareEnable = VK_FALSE;
        createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.mipLodBias = 0.0f;
        createInfo.minLod = 0.0f;
        createInfo.maxLod = 16.0f;

        BX_ASSERT(!vkCreateSampler(device->GetDevice(), &createInfo, nullptr, &this->sampler),
            "Failed to create sampler.");
        DebugNames::Set(*device, VK_OBJECT_TYPE_SAMPLER,
            reinterpret_cast<uint64_t>(this->sampler), name);
    }

    Sampler::~Sampler() {
        vkDestroySampler(this->device->GetDevice(), this->sampler, nullptr);
    }

    VkSampler Sampler::GetSampler() const {
        return this->sampler;
    }
}