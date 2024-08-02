#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/hash_map.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Image;

    struct ImageState {
        VkImageLayout layout;
        VkAccessFlags accessFlags;
        VkPipelineStageFlags stageFlags;
    };

    class ResourceStateTracker {
    public:
        static void TransitionImage(VkCommandBuffer cmdBuffer, const Image& image,
            ImageState newState);

        static void AddGlobalImageState(VkImage image, ImageState state);
        static void RemoveGlobalImageState(VkImage image);

    private:
        static HashMap<VkImage, ImageState> globalImageStates;
    };
}