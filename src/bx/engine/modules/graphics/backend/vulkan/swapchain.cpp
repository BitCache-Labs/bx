#include "bx/engine/modules/graphics/backend/vulkan/swapchain.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/instance.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/render_pass.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/framebuffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/semaphore.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/fence.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/cmd_queue.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/rect2d.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/resource_state_tracker.hpp"

namespace Vk
{
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const Instance& instance,
        const PhysicalDevice& physicalDevice) {
        uint32_t formatCount;
        BX_ENSURE(!vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.GetPhysicalDevice(),
            instance.GetSurface(), &formatCount, nullptr));
        BX_ASSERT(formatCount > 0, "No swapchain formats found.");

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        BX_ENSURE(!vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.GetPhysicalDevice(),
            instance.GetSurface(), &formatCount,
            formats.data()));

        for (const auto& availableFormat : formats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return formats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const Instance& instance,
        const PhysicalDevice& physicalDevice) {
        uint32_t presentModeCount;
        BX_ENSURE(!vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice.GetPhysicalDevice(), instance.GetSurface(), &presentModeCount, nullptr));
        BX_ASSERT(presentModeCount > 0, "No present modes found.");

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        BX_ENSURE(!vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.GetPhysicalDevice(),
            instance.GetSurface(), &presentModeCount,
            presentModes.data()));

        for (const auto& availablePresentMode : presentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(uint32_t width, uint32_t height, const Instance& instance,
        const PhysicalDevice& physicalDevice) {
        VkSurfaceCapabilitiesKHR capabilities;
        BX_ENSURE(!vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.GetPhysicalDevice(),
            instance.GetSurface(), &capabilities));

        VkExtent2D extent;
        if (capabilities.currentExtent.width != UINT_MAX) {
            extent = capabilities.currentExtent;
        }
        else {
            extent = { width, height };

            extent.width = Math::Clamp(extent.width, capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
            extent.height = Math::Clamp(extent.height, capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);
        }

        extent.width = std::max(2U, extent.width);
        extent.height = std::max(2U, extent.height);

        return extent;
    }

    uint32_t ChooseImageCount(const Instance& instance,
        const PhysicalDevice& physicalDevice) {
        VkSurfaceCapabilitiesKHR capabilities;
        BX_ENSURE(!vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.GetPhysicalDevice(),
            instance.GetSurface(), &capabilities));

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }
        return imageCount;
    }

    Swapchain::Swapchain(uint32_t width, uint32_t height, const Instance& instance,
        std::shared_ptr<Device> device,
        const PhysicalDevice& physicalDevice)
        : currentFrame(0), currentImage(0), device(device) {
        this->format = ChooseSwapSurfaceFormat(instance, physicalDevice);
        this->presentMode = ChooseSwapPresentMode(instance, physicalDevice);
        this->extent = ChooseSwapExtent(width, height, instance, physicalDevice);
        this->imageCount = ChooseImageCount(instance, physicalDevice);

        VkSurfaceCapabilitiesKHR capabilities;
        BX_ENSURE(!vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.GetPhysicalDevice(),
            instance.GetSurface(), &capabilities));

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = instance.GetSurface();
        createInfo.minImageCount = this->imageCount;
        createInfo.imageFormat = this->format.format;
        createInfo.imageColorSpace = this->format.colorSpace;
        createInfo.imageExtent = this->extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
#if defined BX_PLATFORM_PC || defined BX_PLATFORM_LINUX
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.preTransform = capabilities.currentTransform;
#else
        BX_FAIL("Unimplemented");
#endif
        createInfo.presentMode = this->presentMode;
        createInfo.clipped = VK_TRUE;

        if (physicalDevice.GraphicsFamily() != physicalDevice.PresentFamily()) {
            uint32_t queueFamilyIndices[] = { physicalDevice.GraphicsFamily(),
                                             physicalDevice.PresentFamily() };

            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        BX_ASSERT(!vkCreateSwapchainKHR(device->GetDevice(), &createInfo, nullptr, &this->swapchain),
            "Failed to create swapchain.");

        vkGetSwapchainImagesKHR(device->GetDevice(), this->swapchain, &this->imageCount, nullptr);
        std::vector<VkImage> swapChainImages(this->imageCount);
        vkGetSwapchainImagesKHR(device->GetDevice(), this->swapchain, &this->imageCount,
            swapChainImages.data());

        std::vector<VkImageView> swapChainImageViews(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = this->format.format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            BX_ASSERT(!vkCreateImageView(device->GetDevice(), &createInfo, nullptr,
                &swapChainImageViews[i]),
                "Failed to create image view.");
        }

        this->images.reserve(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            this->images.push_back(
                std::make_shared<Image>(Log::Format("Swapchain Image {}", i), device, swapChainImages[i], swapChainImageViews[i],
                    this->extent.width, this->extent.height));

            ImageState imageState;
            imageState.layout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            imageState.stageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            imageState.accessFlags =
                VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;  // VkAccessFlagBits::VK_ACCESS_NONE;
            ResourceStateTracker::AddGlobalImageState(this->images[i]->GetImage(), imageState);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            imageAvailableSemaphores.emplace_back(
                Semaphore(Log::Format("Image Available {}", i), device));
            renderFinishedSemaphores.emplace_back(
                Semaphore(Log::Format("Render Finished {}", i), device));
            inFlightFences.emplace_back(std::make_shared<Fence>(Log::Format("Swapchain In Flight Fence {}", i), device, true));
        }

        this->renderPass = std::make_shared<RenderPass>("Swapchain Render Pass",
            this->device, std::vector<VkFormat>{this->Format()}, Optional<VkFormat>::None());
        for (auto& image : this->images) {
            std::vector<std::shared_ptr<Image>> images{ image };
            this->framebuffers.emplace_back(
                Framebuffer("Swapchain Framebuffer", this->device, images, this->renderPass));
        }
    }

    Swapchain::~Swapchain() {
        vkDestroySwapchainKHR(this->device->GetDevice(), this->swapchain, nullptr);
    }

    const Framebuffer& Swapchain::GetCurrentFramebuffer() const {
        BX_ASSERT(!this->framebuffers.empty(), "Rebuild framebuffers first!");
        return this->framebuffers[static_cast<size_t>(this->currentImage)];
    }

    const Image& Swapchain::GetCurrentImage() const {
        return *this->images[static_cast<size_t>(this->currentImage)];
    }

    uint32_t Swapchain::GetCurrentFrameIdx() const {
        return this->currentFrame;
    }

    Semaphore& Swapchain::GetImageAvailableSemaphore() {
        return this->imageAvailableSemaphores[static_cast<size_t>(this->currentFrame)];
    }

    Semaphore& Swapchain::GetRenderFinishedSemaphore() {
        return this->renderFinishedSemaphores[static_cast<size_t>(this->currentFrame)];
    }

    std::shared_ptr<RenderPass> Swapchain::GetRenderPass() {
        return this->renderPass;
    }

    std::shared_ptr<Fence> Swapchain::NextImage() {
        this->inFlightFences[this->currentFrame]->Wait();
        this->inFlightFences[this->currentFrame]->Reset();

        vkAcquireNextImageKHR(
            this->device->GetDevice(), this->swapchain, std::numeric_limits<uint64_t>::max(),
            this->GetImageAvailableSemaphore().GetSemaphore(), VK_NULL_HANDLE, &this->currentImage);

        return this->inFlightFences[this->currentFrame];
    }

    void Swapchain::Present(const CmdQueue& queue, const Fence& fence,
        const std::vector<Semaphore*>& semaphores) {
        std::vector<VkSemaphore> vkSemaphores;
        for (auto& semaphore : semaphores) {
            vkSemaphores.push_back(semaphore->GetSemaphore());
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = static_cast<uint32_t>(vkSemaphores.size());
        presentInfo.pWaitSemaphores = vkSemaphores.data();
        VkSwapchainKHR swapChains[] = { this->swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &this->currentImage;

        vkQueuePresentKHR(queue.GetQueue(), &presentInfo);

        this->currentFrame = (this->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkFormat Swapchain::Format() const {
        return this->format.format;
    }

    Rect2D Swapchain::Extent() const {
        return Rect2D(static_cast<float>(this->extent.width),
            static_cast<float>(this->extent.height));
    }
}