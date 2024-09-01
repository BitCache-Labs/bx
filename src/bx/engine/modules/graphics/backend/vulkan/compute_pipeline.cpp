#include "bx/engine/modules/graphics/backend/vulkan/compute_pipeline.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set_layout.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/shader.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    ComputePipeline::ComputePipeline(
        std::shared_ptr<Device> device, std::shared_ptr<Shader> shader,
        const HashMap<u32, std::shared_ptr<DescriptorSetLayout>>& descriptorSetLayouts)
        : device(device), descriptorSetLayouts(descriptorSetLayouts) {
        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = shader->GetShader();
        computeShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo shaderStages[] = { computeShaderStageInfo };

        std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts{};
        for (auto& descriptorSetLayout : descriptorSetLayouts) {
            vkDescriptorSetLayouts.push_back(descriptorSetLayout.second->GetLayout());
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkDescriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        VK_ASSERT(!vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutInfo, nullptr,
            &this->pipelineLayout),
            "Failed to create pipeline layout.");

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = this->pipelineLayout;
        pipelineInfo.stage = computeShaderStageInfo;

        VK_ASSERT(!vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1,
            &pipelineInfo, nullptr, &this->pipeline),
            "Failed to create compute pipeline.");
    }

    ComputePipeline::~ComputePipeline() {
        vkDestroyPipeline(this->device->GetDevice(), this->pipeline, nullptr);
        vkDestroyPipelineLayout(this->device->GetDevice(), this->pipelineLayout, nullptr);
    }

    VkPipeline ComputePipeline::GetPipeline() const {
        return this->pipeline;
    }

    VkPipelineLayout ComputePipeline::GetLayout() const {
        return this->pipelineLayout;
    }
}