#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class RenderPass;
    class Device;
    class Image;

    class Framebuffer : NoCopy {
    public:
        Framebuffer(const String& name, std::shared_ptr<Device> device,
            List<std::shared_ptr<Image>> images,
            std::shared_ptr<RenderPass> renderPass);
        ~Framebuffer();

        explicit Framebuffer(Framebuffer&& other) noexcept;
        Framebuffer& operator=(Framebuffer&& other) noexcept;

        const List<std::shared_ptr<Image>>& Images() const;

        VkFramebuffer GetFramebuffer() const;

    private:
        VkFramebuffer framebuffer;
        std::shared_ptr<RenderPass> renderPass;
        List<std::shared_ptr<Image>> images;

        const std::shared_ptr<Device> device;
    };
}