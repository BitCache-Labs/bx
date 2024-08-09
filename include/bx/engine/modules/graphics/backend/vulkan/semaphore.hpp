#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
    class Device;

    class Semaphore : NoCopy {
    public:
        Semaphore(const String& name, std::shared_ptr<Device> device);
        ~Semaphore();
        explicit Semaphore(Semaphore&& other) noexcept;
        Semaphore& operator=(Semaphore&& other) noexcept;

        VkSemaphore GetSemaphore() const;

    private:
        VkSemaphore semaphore;

        const std::shared_ptr<Device> device;
    };
}