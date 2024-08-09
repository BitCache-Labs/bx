#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
	class Device;

    class Fence : NoCopy {
    public:
        Fence(const String& name, std::shared_ptr<Device> device, bool signaled = false);
        ~Fence();
        explicit Fence(Fence&& other) noexcept;
        Fence& operator=(Fence&& other) noexcept;

        bool IsComplete() const;
        void Wait() const;
        void Reset();

        VkFence GetFence() const;

    private:
        VkFence fence;

        const std::shared_ptr<Device> device;
    };
}