#include "bx/engine/modules/graphics/backend/vulkan/framebuffer.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/render_pass.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    Framebuffer::Framebuffer(const String& name, std::shared_ptr<Device> device,
        List<std::shared_ptr<Image>> images,
        std::shared_ptr<RenderPass> renderPass)
        : renderPass(renderPass), images(images), device(device) {
        BX_ASSERT(images.size() > 0, "Framebuffer requires at least 1 image.");

        std::vector<VkImageView> attachments;
        for (auto image : images) {
            attachments.push_back(image->GetImageView());
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass->GetRenderPass();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = images[0]->Width();
        framebufferInfo.height = images[0]->Height();
        framebufferInfo.layers = 1;

        BX_ASSERT(!vkCreateFramebuffer(this->device->GetDevice(), &framebufferInfo, nullptr,
            &this->framebuffer),
            "Failed to create frame buffer.");

        DebugNames::Set(*device, VK_OBJECT_TYPE_FRAMEBUFFER, reinterpret_cast<uint64_t>(this->framebuffer),
            name);
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
        : framebuffer(other.framebuffer),
        renderPass(other.renderPass),
        images(other.images),
        device(other.device) {
        other.framebuffer = VK_NULL_HANDLE;
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        this->framebuffer = other.framebuffer;
        this->renderPass = other.renderPass;
        this->images = other.images;
        other.framebuffer = VK_NULL_HANDLE;
        return *this;
    }

    Framebuffer::~Framebuffer() {
        if (this->framebuffer) {
            vkDestroyFramebuffer(this->device->GetDevice(), this->framebuffer, nullptr);
        }
    }

    const List<std::shared_ptr<Image>>& Framebuffer::Images() const {
        return this->images;
    }

    VkFramebuffer Framebuffer::GetFramebuffer() const {
        return this->framebuffer;
    }
}