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
        : name(name), device(device),
        //imageView(VK_NULL_HANDLE),
        width(width),
        height(height),
        depth(depth),
        mips(mips),
        arrayLayers(arrayLayers),
        format(format)
    {
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
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VK_ASSERT(!vmaCreateImage(device->GetAllocator(), &createInfo, &allocCreateInfo,
            &this->image, &this->allocation, nullptr),
            "Failed to create image.");
        DebugNames::Set(*device, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(this->image),
            name);

        ImageState imageState;
        imageState.currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        imageState.lastStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        ResourceStateTracker::AddGlobalImageState(this->image, imageState);
    }

    Image::Image(const String& name, std::shared_ptr<Device> device, VkImage image,
        uint32_t width, uint32_t height, VkFormat format)
        : name(name), device(device),
        image(image),
        allocation(VK_NULL_HANDLE),
        width(width),
        height(height),
        depth(1),
        mips(1),
        arrayLayers(1),
        format(format) {
        DebugNames::Set(*device, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(this->image),
            name);
    }

    Image::~Image() {
        if (this->allocation) {
            ResourceStateTracker::RemoveGlobalImageState(this->image);
            vmaDestroyImage(this->device->GetAllocator(), this->image, this->allocation);
        }
    }

    VkImage Image::GetImage() const {
        return this->image;
    }

    ImageView::ImageView(std::shared_ptr<Device> device, std::shared_ptr<Image> image,
        uint32_t baseMips, uint32_t mips, VkFormat format, uint32_t baseArrayLayer, uint32_t arrayLayers,
        VkImageType dims, uint32_t depth)
        : device(device), image(image)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image->GetImage();
        viewInfo.viewType = (arrayLayers == 1)
            ? ((dims == VK_IMAGE_TYPE_2D) ? VK_IMAGE_VIEW_TYPE_2D
                : VK_IMAGE_VIEW_TYPE_3D)
            : VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask =
            IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = baseMips;
        viewInfo.subresourceRange.levelCount = mips;
        viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
        viewInfo.subresourceRange.layerCount = arrayLayers;

        VK_ASSERT(!vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &this->imageView),
            "Failed to create image view");
    }

    ImageView::~ImageView()
    {
        vkDestroyImageView(this->device->GetDevice(), this->imageView, nullptr);
    }

    std::shared_ptr<Image> ImageView::GetImage() const
    {
        return image;
    }

    VkImageView ImageView::GetImageView() const
    {
        return imageView;
    }
}