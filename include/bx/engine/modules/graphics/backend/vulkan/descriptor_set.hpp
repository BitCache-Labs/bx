#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/array.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;
    class DescriptorPool;
    class DescriptorSetLayout;
    class Buffer;
    class ImageView;
    class Sampler;
    class CmdList;
    class Tlas;

    class DescriptorSet : NoCopy {
    public:
        DescriptorSet(const String& name, const std::shared_ptr<Device> device,
            const std::shared_ptr<DescriptorPool> pool,
            const std::shared_ptr<DescriptorSetLayout> layout);
        ~DescriptorSet();

        void SetBuffer(uint32_t binding, VkDescriptorType type, std::shared_ptr<Buffer> buffer);
        void SetImage(uint32_t binding, VkDescriptorType type, std::shared_ptr<ImageView> image,
            std::shared_ptr<Sampler> sampler);
        void SetImageArray(u32 binding, VkDescriptorType type, const List<std::shared_ptr<ImageView>> images, std::shared_ptr<Sampler> sampler);
        void SetAccelerationStructure(uint32_t binding, std::shared_ptr<Tlas> tlas);

        void TransitionResourceStates(CmdList& cmdList, b8 isGraphics) const;

        VkDescriptorSet GetDescriptorSet() const;

    private:
        VkDescriptorSet descriptorSet;

        Array<std::shared_ptr<Buffer>, 64> trackedBuffers;
        Array<std::shared_ptr<ImageView>, 64> trackedSampledImages;
        Array<std::shared_ptr<ImageView>, 64> trackedStorageImages;
        Array<std::shared_ptr<Sampler>, 64> trackedSamplers;
        Array<std::shared_ptr<Tlas>, 64> trackedAccelerationStructures;

        const std::shared_ptr<Device> device;
        const std::shared_ptr<DescriptorPool> pool;
        const std::shared_ptr<DescriptorSetLayout> layout;
    };
}