#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/list.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;
    class DescriptorSetLayout;
    class Shader;

    class ComputePipeline : NoCopy {
    public:
        ComputePipeline(
            std::shared_ptr<Device> device, std::shared_ptr<Shader> shader,
            List<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayouts);
        ~ComputePipeline();

        VkPipeline GetPipeline() const;
        VkPipelineLayout GetLayout() const;

    private:
        const std::shared_ptr<Device> device;
        const List<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayouts;

        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
    };
}  // namespace Vk