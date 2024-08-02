#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/list.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Instance;
    class Device;
    class PhysicalDevice;
    class RenderPass;
    class Image;
    class Framebuffer;
    class Semaphore;
    class Fence;
    class CmdQueue;
    struct Rect2D;

    class Swapchain : NoCopy {
    public:
        Swapchain(uint32_t width, uint32_t height, const Instance& instance,
            std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice);
        ~Swapchain();

        const Framebuffer& GetCurrentFramebuffer() const;
        const Image& GetCurrentImage() const;
        uint32_t GetCurrentFrameIdx() const;
        Semaphore& GetImageAvailableSemaphore();
        Semaphore& GetRenderFinishedSemaphore();

        std::shared_ptr<RenderPass> GetRenderPass();

        std::shared_ptr<Fence> NextImage();
        void Present(const CmdQueue& queue, const Fence& fence,
            const List<Semaphore*>& semaphores);

        VkFormat Format() const;
        Rect2D Extent() const;

        const static uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    private:
        VkSwapchainKHR swapchain;
        VkSurfaceFormatKHR format;
        VkPresentModeKHR presentMode;
        VkExtent2D extent;

        uint32_t imageCount;
        List<std::shared_ptr<Image>> images;
        List<Framebuffer> framebuffers;

        uint32_t currentFrame;
        uint32_t currentImage;
        List<Semaphore> imageAvailableSemaphores;
        List<Semaphore> renderFinishedSemaphores;
        List<std::shared_ptr<Fence>> inFlightFences;

        std::shared_ptr<RenderPass> renderPass;

        const std::shared_ptr<Device> device;
    };
}