#include "bx/engine/modules/graphics/backend/vulkan/fence.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    Fence::Fence(const String& name, std::shared_ptr<Device> device, bool signaled) : device(device) {
        VkFenceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = signaled ? VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT : 0;

        BX_ASSERT(!vkCreateFence(device->GetDevice(), &createInfo, nullptr, &this->fence),
            "Failed to create fence.");
        DebugNames::Set(*device, VkObjectType::VK_OBJECT_TYPE_FENCE,
            reinterpret_cast<uint64_t>(this->fence), name);
    }

    Fence::Fence(Fence&& other) noexcept : fence(other.fence), device(other.device) {
        other.fence = VK_NULL_HANDLE;
    }

    Fence& Fence::operator=(Fence&& other) noexcept {
        this->fence = other.fence;
        other.fence = VK_NULL_HANDLE;
        return *this;
    }

    Fence::~Fence() {
        if (this->fence) {
            vkDestroyFence(this->device->GetDevice(), this->fence, nullptr);
        }
    }

    bool Fence::IsComplete() const {
        return vkGetFenceStatus(this->device->GetDevice(), this->fence) == VK_SUCCESS;
    }

    void Fence::Wait() const {
        vkWaitForFences(this->device->GetDevice(), 1, &this->fence, true,
            std::numeric_limits<uint64_t>::max());
    }

    void Fence::Reset() {
        vkResetFences(this->device->GetDevice(), 1, &this->fence);
    }

    VkFence Fence::GetFence() const {
        return this->fence;
    }
}