#include "bx/engine/modules/graphics/backend/vulkan/graphics_pipeline.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set_layout.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/render_pass.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/shader.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    GraphicsPipeline::GraphicsPipeline(
        std::shared_ptr<Device> device, const std::vector<const Shader*>& shaders,
        std::shared_ptr<RenderPass> renderPass,
        std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayouts,
        std::vector<PushConstantRange> pushConstants, GraphicsPipelineInfo info)
        : info(info),
        device(device),
        renderPass(renderPass),
        descriptorSetLayouts(descriptorSetLayouts) {
        // TODO: make more dynamic
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = shaders[0]->GetShader();
        vertShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = shaders[1]->GetShader();
        fragShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
                                                     VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // TODO: flexible vertex layouts
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        /*auto vertexBindingDescription = VertexBindingDescription();
        auto vertexAttributeDescriptions = VertexAttributeDescriptions();*/
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        /*if (info.inputVertices) {
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
            vertexInputInfo.vertexAttributeDescriptionCount =
                static_cast<uint32_t>(vertexAttributeDescriptions.size());
            vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
        }*/

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = info.polygonMode;
        rasterizer.cullMode = info.culling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.lineWidth = 1.0;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = info.blending;
        if (info.blending) {
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector<VkPushConstantRange> vkPushConstantRanges{};
        for (auto& pushConstantRange : pushConstants) {
            VkPushConstantRange vkPushConstantRange{};
            vkPushConstantRange.size = static_cast<uint32_t>(pushConstantRange.size);
            vkPushConstantRange.stageFlags = pushConstantRange.stageFlags;
            vkPushConstantRanges.push_back(vkPushConstantRange);
        }

        std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts{};
        for (auto& descriptorSetLayout : descriptorSetLayouts) {
            vkDescriptorSetLayouts.push_back(descriptorSetLayout->GetLayout());
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkDescriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount =
            static_cast<uint32_t>(vkPushConstantRanges.size());
        pipelineLayoutInfo.pPushConstantRanges = vkPushConstantRanges.data();

        BX_ASSERT(!vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutInfo, nullptr,
            &this->pipelineLayout),
            "Failed to create pipeline layout.");

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 0.0f;
        viewport.height = 0.0f;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { 0, 0 };

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = !info.ignoreDepth;
        depthStencil.depthWriteEnable = !info.ignoreDepth;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = this->pipelineLayout;
        pipelineInfo.renderPass = renderPass->GetRenderPass();
        pipelineInfo.subpass = 0;

        BX_ASSERT(!vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
            nullptr, &this->pipeline),
            "Failed to create graphics pipeline.");
    }

    GraphicsPipeline::~GraphicsPipeline() {
        vkDestroyPipeline(this->device->GetDevice(), this->pipeline, nullptr);
        vkDestroyPipelineLayout(this->device->GetDevice(), this->pipelineLayout, nullptr);
    }

    VkPipeline GraphicsPipeline::GetPipeline() const {
        return this->pipeline;
    }

    VkPipelineLayout GraphicsPipeline::GetLayout() const {
        return this->pipelineLayout;
    }
}