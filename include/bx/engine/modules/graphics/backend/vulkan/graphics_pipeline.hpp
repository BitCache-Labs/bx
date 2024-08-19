#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/containers/list.hpp"

#include "vulkan_api.hpp"

#include <memory>

namespace Vk
{
    class DescriptorSetLayout;
    class Device;
    class RenderPass;
    class Shader;

    struct PushConstantRange {
        PushConstantRange(const std::string& name, size_t size, VkShaderStageFlags stageFlags)
            : name(name), size(size), stageFlags(stageFlags) {
        }

        const std::string name;
        const size_t size;
        const VkShaderStageFlags stageFlags;
    };

    struct GraphicsPipelineInfo {
        b8 depthTestEnable = true;
        bool blending = false;
        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkCullModeFlags cullMode = VK_CULL_MODE_NONE;

        List<VkVertexInputBindingDescription> vertexBindingDescriptions{};
        List<VkVertexInputAttributeDescription> vertexAttributeDescriptions{};
    };

    class GraphicsPipeline : NoCopy {
    public:
        GraphicsPipeline(std::shared_ptr<Device> device,
            const List<const Shader*>& shaders,
            std::shared_ptr<RenderPass> renderPass,
            const List<std::shared_ptr<DescriptorSetLayout>>& descriptorSetLayouts,
            List<PushConstantRange> pushConstants, GraphicsPipelineInfo info);
        ~GraphicsPipeline();

        const GraphicsPipelineInfo& Info() const;

        VkPipeline GetPipeline() const;
        VkPipelineLayout GetLayout() const;

    private:
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        GraphicsPipelineInfo info;

        const std::shared_ptr<Device> device;
        const std::shared_ptr<RenderPass> renderPass;
        const List<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayouts;
    };
}