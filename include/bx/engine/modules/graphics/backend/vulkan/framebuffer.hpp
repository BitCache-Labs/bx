#pragma once

#include "bx/engine/core/type.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/hash.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/string.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/render_pass.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;

    struct FramebufferInfo
    {
        List<std::shared_ptr<Image>> images;
        std::shared_ptr<RenderPass> renderPass;

        b8 operator==(const FramebufferInfo& other) const
        {
            if (images.size() != other.images.size())
                return false;

            for (u32 i = 0; i < images.size(); i++)
            {
                if (images[i] != other.images[i])
                    return false;
            }

            return renderPass == other.renderPass;
        }
    };

    class Framebuffer : NoCopy {
    public:
        Framebuffer(const String& name, std::shared_ptr<Device> device,
            const FramebufferInfo& info);
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

template <>
struct std::hash<Vk::FramebufferInfo>
{
    std::size_t operator()(const Vk::FramebufferInfo& v) const
    {
        SizeType result = 0;
        for (auto& image : v.images) {
            const Vk::Image& dimage = *image;
            hashCombine<Vk::Image>(result, dimage);
        }
        hashCombine<Vk::RenderPass>(result, *v.renderPass);
        return result;
    }
};