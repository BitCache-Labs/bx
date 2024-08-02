#include "bx/engine/modules/graphics/backend/vulkan/resource_state_tracker.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image_format.hpp"

namespace Vk
{
    HashMap<VkImage, ImageState> ResourceStateTracker::globalImageStates{};

    void ResourceStateTracker::TransitionImage(VkCommandBuffer cmdBuffer, const Image& image,
        ImageState newState) {
        const ImageState& oldState = globalImageStates.find(image.GetImage())->second;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldState.layout;
        barrier.newLayout = newState.layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = oldState.accessFlags;
        barrier.dstAccessMask = newState.accessFlags;
        barrier.image = image.GetImage();
        if (IsDepthFormat(image.Format()))
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        else
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = image.Mips();
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = image.ArrayLayers();

        VkPipelineStageFlags sourceStage = oldState.stageFlags;
        VkPipelineStageFlags destinationStage = newState.stageFlags;

        vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
            &barrier);

        // TODO: just copied this code from DLR, but it looks like it's broken?
        // Shouldn't it write out the new state to the global states?
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