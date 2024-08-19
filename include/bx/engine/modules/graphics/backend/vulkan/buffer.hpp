#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
	class Device;
	class PhysicalDevice;

    enum class BufferLocation { GPU_ONLY, CPU_TO_GPU };

    class Buffer : NoCopy {
    public:
        Buffer(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, VkBufferUsageFlags usage, uint64_t size,
            BufferLocation location);
        ~Buffer();
        explicit Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        void* Map();
        void Unmap();

        uint64_t Size() const;
        VkBuffer GetBuffer() const;
        VkDeviceAddress GetDeviceAddress() const;

    private:
        uint64_t size;
        VkBuffer buffer;
        VmaAllocation allocation;

        const std::shared_ptr<Device> device;
    };
}