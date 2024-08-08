#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/hash_map.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Image;

    struct ImageState {
        VkImageLayout currentLayout;
        VkPipelineStageFlags lastStageFlags;

        b8 operator==(const ImageState& other) const
        {
            return currentLayout == other.currentLayout && lastStageFlags == other.lastStageFlags;
        }
    };

    class ResourceStateTracker {
    public:
        static void TransitionImage(VkCommandBuffer cmdBuffer, const Image& image,
            VkImageLayout newLayout, VkPipelineStageFlags newStage);

        static void AddGlobalImageState(VkImage image, ImageState state);
        static void RemoveGlobalImageState(VkImage image);

    private:
        static HashMap<VkImage, ImageState> globalImageStates;
    };
}