#include "bx/engine/modules/graphics/backend/vulkan/resource_state_tracker.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image_format.hpp"

namespace Vk
{
    HashMap<VkImage, ImageState> ResourceStateTracker::globalImageStates{};

    VkAccessFlags LayoutToAccessFlags(VkImageLayout layout)
    {
        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return VK_ACCESS_NONE;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_ACCESS_NONE;// VK_ACCESS_NONE;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;// | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;// | VK_ACCESS_HOST_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;

        default:
            BX_FAIL("Unsupported image layout");
            return VK_ACCESS_NONE;
        }
    }

    void ResourceStateTracker::TransitionImage(VkCommandBuffer cmdBuffer, const Image& image,
        VkImageLayout newLayout, VkPipelineStageFlags newStage) {
        ImageState& state = globalImageStates.find(image.GetImage())->second;

        if (state.currentLayout == newLayout)
            return;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = state.currentLayout;
        barrier.newLayout = newLayout;
        barrier.srcAccessMask = LayoutToAccessFlags(state.currentLayout);
        barrier.dstAccessMask = LayoutToAccessFlags(newLayout);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.GetImage();

        if (IsDepthFormat(image.Format()) || IsStencilFormat(image.Format()))
        {
            if (IsDepthFormat(image.Format()))
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (IsStencilFormat(image.Format()))
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = image.Mips();
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = image.ArrayLayers();

        VkPipelineStageFlags sourceStage = state.lastStageFlags;
        VkPipelineStageFlags destinationStage = newStage;

        vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
            &barrier);

        state.currentLayout = newLayout;
        state.lastStageFlags = newStage;
    }

    VkImageLayout ResourceStateTracker::GetCurrentImageLayout(const Image& image)
    {
        ImageState& state = globalImageStates.find(image.GetImage())->second;
        return state.currentLayout;
    }

    void ResourceStateTracker::ApplyImplicitImageTransition(const Image& image, VkImageLayout newLayout)
    {
        ImageState& state = globalImageStates.find(image.GetImage())->second;
        state.currentLayout = newLayout;
    }

    void ResourceStateTracker::AddGlobalImageState(VkImage image, ImageState state) {
        if (globalImageStates.find(image) == globalImageStates.end())
            globalImageStates.insert(std::make_pair(image, state));
        else
            BX_LOGW("Adding image state to tracker twice.");
    }

    void ResourceStateTracker::RemoveGlobalImageState(VkImage image) {
        globalImageStates.erase(image);
    }
}