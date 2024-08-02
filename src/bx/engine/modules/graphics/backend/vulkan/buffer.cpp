#include "bx/engine/modules/graphics/backend/vulkan/buffer.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    Buffer::Buffer(const std::string& name, std::shared_ptr<Device> device,
        const PhysicalDevice& physicalDevice, VkBufferUsageFlags usage, uint64_t size,
        BufferLocation location)
        : size(size), device(device) {
        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = size;
        createInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
        createInfo.usage = usage;

        VmaAllocationCreateInfo allocCreateInfo{};
        if (physicalDevice.RayTracingSuitable()) {
            allocCreateInfo.flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            createInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }
        if (location == BufferLocation::CPU_TO_GPU) {
            allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        BX_ASSERT(!vmaCreateBuffer(device->GetAllocator(), &createInfo, &allocCreateInfo,
            &this->buffer, &this->allocation, nullptr),
            "Failed to create buffer.");
        DebugNames::Set(*device, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(this->buffer),
            name);
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : size(other.size),
        buffer(other.buffer),
        allocation(other.allocation),
        device(other.device) {
        other.buffer = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept {
        this->size = other.size;
        this->buffer = other.buffer;
        this->allocation = other.allocation;
        other.buffer = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
        return *this;
    }

    Buffer::~Buffer() {
        if (this->buffer) {
            vmaDestroyBuffer(this->device->GetAllocator(), this->buffer, this->allocation);
        }
    }

    void* Buffer::Map() {
        void* data;
        vmaMapMemory(this->device->GetAllocator(), this->allocation, &data);
        return data;
    }

    void Buffer::Unmap() {
        vmaUnmapMemory(this->device->GetAllocator(), this->allocation);
        vmaFlushAllocation(this->device->GetAllocator(), this->allocation, 0, this->size);
    }

    uint64_t Buffer::Size() const {
        return this->size;
    }

    VkBuffer Buffer::GetBuffer() const {
        return this->buffer;
    }

    VkDeviceAddress Buffer::GetDeviceAddress() const {
        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = this->buffer;
        info.pNext = nullptr;

        VkDevice device = this->device->GetDevice();
        const VkBufferDeviceAddressInfo* pInfo = &info;
        return vkGetBufferDeviceAddress(device, pInfo);
    }
}