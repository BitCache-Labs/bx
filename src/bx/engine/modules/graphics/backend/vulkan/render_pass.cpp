#include "bx/engine/modules/graphics/backend/vulkan/render_pass.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/resource_state_tracker.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image_format.hpp"

namespace Vk
{
    RenderPass::RenderPass(const String& name, std::shared_ptr<Device> device,
        const RenderPassInfo& info)
        : device(device) {
        List<VkAttachmentDescription> attachements;

        List<VkAttachmentReference> colorAttachmentRefs;
        for (size_t i = 0; i < info.colorFormats.size(); i++) {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = info.colorFormats[i];
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = info.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = static_cast<uint32_t>(i);
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachements.push_back(colorAttachment);
            colorAttachmentRefs.push_back(colorAttachmentRef);
        }

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = static_cast<uint32_t>(colorAttachmentRefs.size());
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        if (info.depthFormat.IsSome()) {
            VkImageLayout depthStencilLayout = GetDepthStencilLayout(info.depthFormat.Unwrap());
            
            depthAttachmentRef.layout = depthStencilLayout;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = info.depthFormat.Unwrap();
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = info.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.initialLayout = depthStencilLayout;
            depthAttachment.finalLayout = depthStencilLayout;
            attachements.push_back(depthAttachment);
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = info.depthFormat.IsSome() ? &depthAttachmentRef : nullptr;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachements.size());
        renderPassInfo.pAttachments = attachements.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VK_ASSERT(
            !vkCreateRenderPass(device->GetDevice(), &renderPassInfo, nullptr, &this->renderPass),
            "Failed to create render pass.");
        
        DebugNames::Set(*device, VkObjectType::VK_OBJECT_TYPE_RENDER_PASS,
            reinterpret_cast<uint64_t>(this->renderPass), name);
    }

    RenderPass::~RenderPass() {
        vkDestroyRenderPass(this->device->GetDevice(), this->renderPass, nullptr);
    }

    VkRenderPass RenderPass::GetRenderPass() const {
        return this->renderPass;
    }
}