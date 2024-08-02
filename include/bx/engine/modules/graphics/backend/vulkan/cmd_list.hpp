#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/math.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class CmdQueue;
    class Device;
    class Buffer;
    class Image;
    class GraphicsPipeline;
    struct Rect2D;
    class Framebuffer;
    class RenderPass;
    class DescriptorSet;

    class CmdList : NoCopy {
    public:
        ~CmdList();

        VkCommandBuffer GetCommandBuffer() const;

        void CopyBuffers(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst);
        void CopyBuffers(std::shared_ptr<Buffer> src, std::shared_ptr<Image> dst);
        void CopyImages(std::shared_ptr<Image> src, std::shared_ptr<Image> dst);
        void CopyImagesIntoCubemap(const std::array<std::shared_ptr<Image>, 6>& images,
            std::shared_ptr<Image> cubemap);
        void GenerateMips(std::shared_ptr<Image> image);

        void TransitionImageLayout(std::shared_ptr<Image> image, VkImageLayout layout,
            VkAccessFlags access, VkShaderStageFlags pipelineStage);

        void SetScissor(const Rect2D& rect2D);
        void SetViewport(const Rect2D& rect2D, bool normalize = true);

        void BindVertexBuffer(std::shared_ptr<Buffer> vertexBuffer);
        void BindIndexBuffer(std::shared_ptr<Buffer> indexBuffer);

        /*void BuildBLAS(VkAccelerationStructureGeometryKHR geometry,
            std::shared_ptr<Buffer> scratchBuffer, std::shared_ptr<Buffer> resultBuffer,
            VkAccelerationStructureKHR blas, uint32_t indexCount);*/

        void BeginRenderPass(std::shared_ptr<RenderPass> renderPass, const Framebuffer& framebuffer,
            const Color& clearColor);
        void EndRenderPass();

        void BindDescriptorSet(std::shared_ptr<DescriptorSet> descriptorSet, uint32_t set,
            VkPipelineBindPoint pipelineType = VK_PIPELINE_BIND_POINT_GRAPHICS);

        void BindGraphicsPipeline(std::shared_ptr<GraphicsPipeline> graphicsPipeline);
        //void BindComputePipeline(std::shared_ptr<ComputePipeline> computePipeline);

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0,
            uint32_t firstInstance = 0);
        void DrawElements(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0,
            uint32_t firstInstance = 0, uint32_t vertexOffset = 0);

        void Dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1);

        template <typename T>
        void PushConstant(const std::string& name, T& constant, VkShaderStageFlags stageFlags) {
            this->PushConstant(name, static_cast<void*>(&constant), sizeof(T), stageFlags);
        }

    private:
        friend class CmdQueue;
        CmdList(std::shared_ptr<Device> device, const CmdQueue& cmdQueue);

        const std::shared_ptr<Device> device;
        const CmdQueue& cmdQueue;

        VkCommandBuffer cmdBuffer;

        void Begin();
        void End();
        void Reset();

        void PushConstant(const std::string& name, void* constant, size_t size,
            VkShaderStageFlags stageFlags);

        std::shared_ptr<GraphicsPipeline> boundGraphicsPipeline;
        //std::shared_ptr<ComputePipeline> boundComputePipeline;
        std::vector<std::shared_ptr<RenderPass>> trackedRenderPasses;
        std::vector<std::shared_ptr<Buffer>> trackedBuffers;
        std::vector<std::shared_ptr<Image>> trackedImages;
        std::vector<std::shared_ptr<DescriptorSet>> trackedDescriptorSets;
    };
}