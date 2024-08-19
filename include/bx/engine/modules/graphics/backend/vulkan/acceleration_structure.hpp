#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
	class Device;
    class PhysicalDevice;
    class Buffer;
    struct RTProperties;

    class AccelerationStructure : NoCopy {
    public:
        ~AccelerationStructure();

        VkAccelerationStructureKHR GetAccelerationStructure() const;
        const VkAccelerationStructureBuildSizesInfoKHR GetBuildSizes() const;

    protected:
        AccelerationStructure(const String& name, std::shared_ptr<Device> device,
                              const PhysicalDevice& physicalDevice);

        VkAccelerationStructureBuildSizesInfoKHR CalculateBuildSizes(
            uint32_t maxPrimitiveCounts) const;
        void CreateAccelerationStructure(Buffer& resultBuffer, VkDeviceSize resultOffset);

        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo;

    private:
        VkAccelerationStructureKHR accelerationStructure;

        const std::shared_ptr<Device> device;
        const RTProperties& rtProperties;
    };
}