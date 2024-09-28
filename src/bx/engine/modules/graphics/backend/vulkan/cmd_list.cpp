#include "bx/engine/modules/graphics/backend/vulkan/cmd_list.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/cmd_queue.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/buffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/pfn.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/graphics_pipeline.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/compute_pipeline.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/rect2d.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/render_pass.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/framebuffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/resource_state_tracker.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    CmdList::CmdList(const String& name, std::shared_ptr<Device> device, const CmdQueue& cmdQueue)
        : name(name), device(device), cmdQueue(cmdQueue) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = cmdQueue.GetCmdPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VK_ASSERT(!vkAllocateCommandBuffers(device->GetDevice(), &allocInfo, &this->cmdBuffer),
            "Failed to allocate command buffer.");
    }

    CmdList::~CmdList() {
        vkFreeCommandBuffers(this->device->GetDevice(), cmdQueue.GetCmdPool(), 1,
            &this->cmdBuffer);
    }

    VkCommandBuffer CmdList::GetCommandBuffer() const
    {
        return this->cmdBuffer;
    }

    void CmdList::Begin() {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_ASSERT(!vkBeginCommandBuffer(this->cmdBuffer, &beginInfo),
            "Failed to begin command buffer.");
    }

    void CmdList::End() {
        VK_ASSERT(!vkEndCommandBuffer(this->cmdBuffer), "Failed to end command buffer.");
    }

    void CmdList::Reset() {
        vkResetCommandBuffer(this->cmdBuffer, 0);

        this->boundGraphicsPipeline.reset();
        this->boundComputePipeline.reset();
        this->trackedRenderPasses.clear();
        this->trackedBuffers.clear();
        this->trackedImages.clear();
        this->trackedFramebuffers.clear();
        this->trackedDescriptorSets.clear();
    }

    void CmdList::FillBuffer(std::shared_ptr<Buffer> dst, u32 value)
    {
        vkCmdFillBuffer(this->cmdBuffer, dst->GetBuffer(), 0, dst->Size(), value);

        this->trackedBuffers.push_back(dst);
    }

    void CmdList::CopyBuffers(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst, u32 dstOffset) {
        VK_ASSERT((src->Size() + dstOffset) <= dst->Size(), "Copy buffers src size + dst offset must be less than dst size.");

        VkBufferCopy copyRegion{};
        copyRegion.size = src->Size();
        copyRegion.dstOffset = dstOffset;

        vkCmdCopyBuffer(this->cmdBuffer, src->GetBuffer(), dst->GetBuffer(), 1, &copyRegion);

        this->trackedBuffers.push_back(src);
        this->trackedBuffers.push_back(dst);
    }

    void CmdList::CopyBuffers(std::shared_ptr<Buffer> src, std::shared_ptr<Image> dst) {
        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageOffset = { 0, 0, 0 };
        copyRegion.imageExtent = { dst->Width(), dst->Height(), dst->Depth() };

        vkCmdCopyBufferToImage(this->cmdBuffer, src->GetBuffer(), dst->GetImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        this->trackedBuffers.push_back(src);
        this->trackedImages.push_back(dst);
    }

    void CmdList::CopyBuffers(std::shared_ptr<Image> src, std::shared_ptr<Buffer> dst, VkOffset3D offset, VkExtent3D size) {
        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageOffset = offset;
        copyRegion.imageExtent = size;

        vkCmdCopyImageToBuffer(this->cmdBuffer, src->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->GetBuffer(), 1, &copyRegion);

        this->trackedBuffers.push_back(dst);
        this->trackedImages.push_back(src);
    }

    void CmdList::CopyImages(std::shared_ptr<Image> src, std::shared_ptr<Image> dst) {
        this->TransitionImageLayout(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
        this->TransitionImageLayout(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { static_cast<int32_t>(src->Width()),
                              static_cast<int32_t>(src->Height()),
                              static_cast<int32_t>(src->Depth()) };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { static_cast<int32_t>(dst->Width()),
                              static_cast<int32_t>(dst->Height()),
                              static_cast<int32_t>(dst->Depth()) };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = 0;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(this->cmdBuffer, src->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
            VK_FILTER_LINEAR);

        this->trackedImages.push_back(src);
        this->trackedImages.push_back(dst);
    }

    void CmdList::CopyImagesIntoCubemap(const std::array<std::shared_ptr<Image>, 6>& images,
        std::shared_ptr<Image> cubemap) {
        this->TransitionImageLayout(cubemap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageCopy imageCopy{};
        imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageCopy.srcSubresource.baseArrayLayer = 0;
        imageCopy.srcSubresource.layerCount = 1;
        imageCopy.srcOffset = { 0, 0, 0 };
        imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageCopy.dstSubresource.layerCount = 1;
        imageCopy.dstOffset = { 0, 0, 0 };

        uint32_t mipWidth = cubemap->Width();
        uint32_t mipHeight = cubemap->Height();

        for (size_t i = 0; i < images.size(); i++) {
            this->TransitionImageLayout(images[i], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        for (uint32_t mip = 0; mip < images[0]->Mips(); mip++) {
            imageCopy.srcSubresource.mipLevel = mip;
            imageCopy.dstSubresource.mipLevel = mip;

            imageCopy.extent = { mipWidth, mipHeight, 1 };

            for (size_t i = 0; i < images.size(); i++) {
                imageCopy.dstSubresource.baseArrayLayer = static_cast<uint32_t>(i);

                vkCmdCopyImage(this->cmdBuffer, images[i]->GetImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cubemap->GetImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            }

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }

        this->TransitionImageLayout(cubemap, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    void CmdList::GenerateMips(std::shared_ptr<Image> image) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image->GetImage();
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = static_cast<int32_t>(image->Width());
        int32_t mipHeight = static_cast<int32_t>(image->Height());
        int32_t mipDepth = static_cast<int32_t>(image->Depth());

        this->TransitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);

        for (uint32_t i = 1; i < image->Mips(); i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = ResourceStateTracker::GetCurrentImageLayout(*image);
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(this->cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, mipDepth };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
                                  mipHeight > 1 ? mipHeight / 2 : 1,
                                  mipDepth > 1 ? mipDepth / 2 : 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(this->cmdBuffer, image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(this->cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                1, &barrier);

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
            if (mipDepth > 1)
                mipDepth /= 2;
        }

        barrier.subresourceRange.baseMipLevel = image->Mips() - 1;
        barrier.oldLayout = ResourceStateTracker::GetCurrentImageLayout(*image);
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(this->cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &barrier);

        ResourceStateTracker::ApplyImplicitImageTransition(*image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        this->trackedImages.push_back(image);
    }

    void CmdList::TransitionImageLayout(std::shared_ptr<Image> image, VkImageLayout layout, VkPipelineStageFlags pipelineStage) {
        ResourceStateTracker::TransitionImage(this->cmdBuffer, *image, layout, pipelineStage);
        this->trackedImages.push_back(image);
    }

    void CmdList::SetScissor(const Rect2D& rect2D) {
        VkRect2D vkRect{};
        vkRect.offset.x = static_cast<int32_t>(rect2D.offset.x);
        vkRect.offset.y = static_cast<int32_t>(rect2D.offset.y);
        vkRect.extent.width = static_cast<uint32_t>(rect2D.extent.x);
        vkRect.extent.height = static_cast<uint32_t>(rect2D.extent.y);

        vkCmdSetScissor(this->cmdBuffer, 0, 1, &vkRect);
    }

    void CmdList::SetViewport(const Rect2D& rect2D, bool normalize) {
        VkViewport viewport{};
        if (normalize) {
            viewport.width = rect2D.extent.x;
            viewport.height = -rect2D.extent.y;
            viewport.x = rect2D.offset.x;
            viewport.y = rect2D.offset.y - viewport.height;
        }
        else {
            viewport.width = rect2D.extent.x;
            viewport.height = rect2D.extent.y;
            viewport.x = rect2D.offset.x;
            viewport.y = rect2D.offset.y;
        }
        viewport.maxDepth = 1.0;

        vkCmdSetViewport(this->cmdBuffer, 0, 1, &viewport);
    }

    void CmdList::BindVertexBuffer(std::shared_ptr<Buffer> vertexBuffer) {
        VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(this->cmdBuffer, 0, 1, vertexBuffers, offsets);

        this->trackedBuffers.push_back(vertexBuffer);
    }

    void CmdList::BindIndexBuffer(std::shared_ptr<Buffer> indexBuffer, VkIndexType type) {
        vkCmdBindIndexBuffer(this->cmdBuffer, indexBuffer->GetBuffer(), 0, type);

        this->trackedBuffers.push_back(indexBuffer);
    }

    void CmdList::BuildAccelerationStructure(VkAccelerationStructureBuildGeometryInfoKHR buildInfo, const VkAccelerationStructureBuildRangeInfoKHR& rangeInfo,
        std::shared_ptr<Buffer> scratchBuffer, std::shared_ptr<Buffer> resultBuffer,
        VkAccelerationStructureKHR accelerationStructure)
    {
        /*VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
        buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildGeometryInfo.geometryCount = 1;
        buildGeometryInfo.pGeometries = &geometry;
        buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildGeometryInfo.dstAccelerationStructure = blas;
        buildGeometryInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress();

        VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo{};
        buildOffsetInfo.primitiveCount = indexCount / 3;
        const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;*/

        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        buildInfo.dstAccelerationStructure = accelerationStructure;
        buildInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress();

        auto pRangeInfo = &rangeInfo;

        Pfn::vkCmdBuildAccelerationStructuresKHR(this->cmdBuffer, 1, &buildInfo, &pRangeInfo);

        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(this->cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        this->trackedBuffers.push_back(scratchBuffer);
        this->trackedBuffers.push_back(resultBuffer);
    }

    void CmdList::UpdateAccelerationStructure(VkAccelerationStructureBuildGeometryInfoKHR buildInfo, const VkAccelerationStructureBuildRangeInfoKHR& rangeInfo,
        std::shared_ptr<Buffer> scratchBuffer, std::shared_ptr<Buffer> resultBuffer, VkAccelerationStructureKHR accelerationStructure)
    {
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        buildInfo.srcAccelerationStructure = accelerationStructure;
        buildInfo.dstAccelerationStructure = accelerationStructure;
        buildInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress();

        auto pRangeInfo = &rangeInfo;

        Pfn::vkCmdBuildAccelerationStructuresKHR(this->cmdBuffer, 1, &buildInfo, &pRangeInfo);

        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(this->cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        this->trackedBuffers.push_back(scratchBuffer);
        this->trackedBuffers.push_back(resultBuffer);
    }

    void CmdList::BeginRenderPass(std::shared_ptr<RenderPass> renderPass,
        std::shared_ptr<Framebuffer> framebuffer, const Color& clearColor) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass->GetRenderPass();
        renderPassInfo.framebuffer = framebuffer->GetFramebuffer();
        renderPassInfo.renderArea.extent =
            VkExtent2D{ framebuffer->Images()[0]->Width(), framebuffer->Images()[0]->Height() };

        List<VkClearValue> clearValues{};
        for (u32 i = 0; i < renderPass->GetInfo().colorFormats.size(); i++)
        {
            VkClearValue clearValue{};
            clearValue.color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
            clearValues.push_back(clearValue);
        }
        if (renderPass->GetInfo().depthFormat.IsSome())
        {
            VkClearValue clearValue{};
            clearValue.depthStencil = { 1.0f, 0 };
            clearValues.push_back(clearValue);
        }

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(this->cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        this->trackedRenderPasses.push_back(renderPass);
        this->trackedFramebuffers.push_back(framebuffer);
    }

    void CmdList::EndRenderPass() {
        vkCmdEndRenderPass(this->cmdBuffer);
    }

    void CmdList::BindDescriptorSet(std::shared_ptr<DescriptorSet> descriptorSet, uint32_t set,
        VkPipelineBindPoint pipelineType) {
        VkDescriptorSet descriptorSets[] = { descriptorSet->GetDescriptorSet() };

        VkPipelineLayout layout = (pipelineType == VK_PIPELINE_BIND_POINT_GRAPHICS)
            ? this->boundGraphicsPipeline->GetLayout()
            : this->boundComputePipeline->GetLayout();
        vkCmdBindDescriptorSets(this->cmdBuffer, pipelineType, layout, set, 1, descriptorSets, 0,
            nullptr);

        trackedDescriptorSets.push_back(descriptorSet);
    }

    void CmdList::BindGraphicsPipeline(std::shared_ptr<GraphicsPipeline> graphicsPipeline) {
        vkCmdBindPipeline(this->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline->GetPipeline());

        this->boundGraphicsPipeline = graphicsPipeline;
    }

    void CmdList::BindComputePipeline(std::shared_ptr<ComputePipeline> computePipeline) {
        vkCmdBindPipeline(this->cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            computePipeline->GetPipeline());

        this->boundComputePipeline = computePipeline;
    }

    void CmdList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
        uint32_t firstInstance) {
        vkCmdDraw(this->cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CmdList::DrawElements(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
        uint32_t firstInstance, uint32_t vertexOffset) {
        vkCmdDrawIndexed(this->cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset,
            firstInstance);
    }

    void CmdList::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
        vkCmdDispatch(this->cmdBuffer, x, y, z);
    }

    void CmdList::DispatchIndirect(std::shared_ptr<Buffer> indirectArgs, u32 offset)
    {
        vkCmdDispatchIndirect(this->cmdBuffer, indirectArgs->GetBuffer(), offset);

        this->trackedBuffers.push_back(indirectArgs);
    }

    void CmdList::PushConstant(const std::string& name, void* constant, size_t size,
        VkShaderStageFlags stageFlags) {
        vkCmdPushConstants(this->cmdBuffer, this->boundGraphicsPipeline->GetLayout(),
            stageFlags, 0, static_cast<uint32_t>(size),
            constant);
    }
}