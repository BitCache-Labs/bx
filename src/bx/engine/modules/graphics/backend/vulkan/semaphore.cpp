#include "bx/engine/modules/graphics/backend/vulkan/semaphore.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    Semaphore::Semaphore(const std::string& name, std::shared_ptr<Device> device)
        : device(device) {
        VkSemaphoreCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        BX_ASSERT(!vkCreateSemaphore(device->GetDevice(), &createInfo, nullptr, &this->semaphore),
            "Failed to create semaphore.");
        DebugNames::Set(*device, VkObjectType::VK_OBJECT_TYPE_SEMAPHORE,
            reinterpret_cast<uint64_t>(this->semaphore), name);
    }

    Semaphore::Semaphore(Semaphore&& other) noexcept
        : semaphore(other.semaphore), device(other.device) {
        other.semaphore = VK_NULL_HANDLE;
    }

    Semaphore& Semaphore::operator=(Semaphore&& other) noexcept {
        this->semaphore = other.semaphore;
        other.semaphore = VK_NULL_HANDLE;
        return *this;
    }

    Semaphore::~Semaphore() {
        if (this->semaphore) {
            vkDestroySemaphore(this->device->GetDevice(), this->semaphore, nullptr);
        }
    }

    VkSemaphore Semaphore::GetSemaphore() const {
        return this->semaphore;
    }
}