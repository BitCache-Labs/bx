#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
	class Device;
	class PhysicalDevice;

    class Image : NoCopy {
    public:
        Image(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, uint32_t mips,
            VkImageUsageFlags usage, VkFormat format, uint32_t arrayLayers = 1,
            VkImageType type = VK_IMAGE_TYPE_2D, uint32_t depth = 1);
        Image(const String& name, std::shared_ptr<Device> device, VkImage image, VkImageView imageView,
            uint32_t width, uint32_t height);
        ~Image();

        uint32_t Width() const;
        uint32_t Height() const;
        uint32_t Depth() const;
        uint32_t Mips() const;
        uint32_t ArrayLayers() const;
        VkFormat Format() const;

        VkImage GetImage() const;
        VkImageView GetImageView() const;

    private:
        const std::shared_ptr<Device> device;

        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;

        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t mips;
        uint32_t arrayLayers;
        VkFormat format;
    };
}