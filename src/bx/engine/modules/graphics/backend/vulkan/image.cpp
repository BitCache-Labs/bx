#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image_format.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/resource_state_tracker.hpp"

namespace Vk
{
    uint32_t Image::Width() const {
        return this->width;
    }

    uint32_t Image::Height() const {
        return this->height;
    }

    uint32_t Image::Depth() const {
        return this->depth;
    }

    uint32_t Image::Mips() const {
        return this->mips;
    }

    uint32_t Image::ArrayLayers() const {
        return this->arrayLayers;
    }

    VkFormat Image::Format() const {
        return this->format;
    }

    Image::Image(const String& name, std::shared_ptr<Device> device,
        const PhysicalDevice& physicalDevice, uint32_t width, uint32_t height,
        uint32_t mips, VkImageUsageFlags usage, VkFormat format, uint32_t arrayLayers,
        VkImageType dims, unsigned depth)
        : device(device),
        imageView(VK_NULL_HANDLE),
        width(width),
        height(height),
        depth(depth),
        mips(mips),
        arrayLayers(arrayLayers),
        format(format) {
        VkImageCreateInfo createInfo{};
        createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.imageType = dims;
        createInfo.extent.width = width;
        createInfo.extent.height = height;
        createInfo.extent.depth = depth;
        createInfo.mipLevels = mips;
        createInfo.format = format;
        createInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
        createInfo.arrayLayers = arrayLayers;
        createInfo.usage = usage;
        createInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        createInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
        if (arrayLayers > 1)
            createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        BX_ASSERT(!vmaCreateImage(device->GetAllocator(), &createInfo, &allocCreateInfo,
            &this->image, &this->allocation, nullptr),
            "Failed to create image.");
        DebugNames::Set(*device, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(this->image),
            name);

        ImageState imageState;
        imageState.layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        imageState.stageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        imageState.accessFlags = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;//VkAccessFlagBits::VK_ACCESS_NONE;//
        ResourceStateTracker::AddGlobalImageState(this->image, imageState);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = this->image;
        viewInfo.viewType = (arrayLayers == 1)
            ? ((dims == VK_IMAGE_TYPE_2D) ? VK_IMAGE_VIEW_TYPE_2D
                : VK_IMAGE_VIEW_TYPE_3D)
            : VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask =
            IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mips;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = arrayLayers;

        BX_ASSERT(!vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &this->imageView),
            "Failed to create image view");
    }

    Image::Image(const String& name, std::shared_ptr<Device> device, VkImage image, VkImageView imageView,
        uint32_t width, uint32_t height)
        : device(device),
        image(image),
        imageView(imageView),
        allocation(VK_NULL_HANDLE),
        width(width),
        height(height),
        depth(1),
        mips(1),
        arrayLayers(1) {
        DebugNames::Set(*device, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(this->image),
            name);
    }

    Image::~Image() {
        vkDestroyImageView(this->device->GetDevice(), this->imageView, nullptr);

        if (this->allocation) {
            ResourceStateTracker::RemoveGlobalImageState(this->image);
            vmaDestroyImage(this->device->GetAllocator(), this->image, this->allocation);
        }
    }

    VkImage Image::GetImage() const {
        return this->image;
    }

    VkImageView Image::GetImageView() const {
        return this->imageView;
    }
}