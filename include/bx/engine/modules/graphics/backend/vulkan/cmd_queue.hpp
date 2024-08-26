#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/containers/list.hpp"

#include "vulkan_api.hpp"

#include <queue>
#include <thread>
#include <mutex>

namespace Vk
{
    class Device;
    class PhysicalDevice;
    class CmdList;
    class Semaphore;
    class Fence;

    enum class QueueType { GRAPHICS, COMPUTE, PRESENT };

    class CmdQueue : NoCopy {
    public:
        CmdQueue(const std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
            QueueType type);
        ~CmdQueue();
        
        std::shared_ptr<CmdList> GetCmdList(const String& name);
        void SubmitCmdList(std::shared_ptr<CmdList> cmdList, std::shared_ptr<Fence> fence,
            const List<Semaphore*>& waitSemaphores,
            const List<VkPipelineStageFlags>& waitStages,
            const List<Semaphore*>& signalSemaphores);

        VkQueue GetQueue() const;
        VkCommandPool GetCmdPool() const;

    private:
        VkQueue queue;
        VkCommandPool cmdPool;

        const std::shared_ptr<Device> device;

        struct InFlightCmdList {
            std::shared_ptr<Fence> fence;
            std::shared_ptr<CmdList> cmdList;
        };

        List<InFlightCmdList> busyCmdLists;
        std::queue<std::shared_ptr<CmdList>> idleCmdLists;

        void ProcessCmdLists();
        std::mutex busyCmdListsMutex;
        std::mutex processCmdListsThreadMutex;
        std::thread processCmdListsThread;
        b8 shouldProcessCmdLists = true;
    };
}