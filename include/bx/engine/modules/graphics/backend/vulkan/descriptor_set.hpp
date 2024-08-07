#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/array.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;
    class DescriptorPool;
    class DescriptorSetLayout;
    class Buffer;
    class Image;
    class Sampler;
    class CmdList;

    class DescriptorSet : NoCopy {
    public:
        DescriptorSet(const String& name, const std::shared_ptr<Device> device,
            const std::shared_ptr<DescriptorPool> pool,
            const std::shared_ptr<DescriptorSetLayout> layout);
        ~DescriptorSet();

        void SetBuffer(uint32_t binding, VkDescriptorType type, std::shared_ptr<Buffer> buffer);
        void SetImage(uint32_t binding, VkDescriptorType type, std::shared_ptr<Image> image,
            std::shared_ptr<Sampler> sampler);

        void TransitionResourceStates(std::shared_ptr<CmdList> cmdList) const;

        VkDescriptorSet GetDescriptorSet() const;

    private:
        VkDescriptorSet descriptorSet;

        Array<std::shared_ptr<Buffer>, 128> trackedBuffers;
        Array<std::shared_ptr<Image>, 128> trackedImages;
        Array<std::shared_ptr<Sampler>, 128> trackedSamplers;

        const std::shared_ptr<Device> device;
        const std::shared_ptr<DescriptorPool> pool;
        const std::shared_ptr<DescriptorSetLayout> layout;
    };
}