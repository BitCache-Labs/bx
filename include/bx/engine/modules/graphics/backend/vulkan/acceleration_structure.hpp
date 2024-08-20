#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
	class Device;
    class PhysicalDevice;
    class Buffer;
    class CmdList;

    class AccelerationStructure : NoCopy
    {
    public:
        ~AccelerationStructure();

    protected:
        AccelerationStructure(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, u32 size, VkAccelerationStructureTypeKHR type);

        const std::shared_ptr<Device> device;
        const PhysicalDevice& physicalDevice;

        VkAccelerationStructureKHR GetAccelerationStructure() const;
        std::shared_ptr<Buffer> GetBuffer() const;

    private:
        VkAccelerationStructureKHR accelerationStructure;
        std::shared_ptr<Buffer> buffer;
    };

    class Blas : AccelerationStructure
    {
    public:
        Blas(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, u32 size);

        static u32 RequiredSize(std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
            VkAccelerationStructureGeometryKHR geometry, u32 maxPrimitiveCounts, VkBuildAccelerationStructureFlagsKHR flags);

        void Build(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags);
        void Update(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags);
    };

    class Tlas : AccelerationStructure
    {
    public:
        Tlas(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, u32 size);

        static u32 RequiredSize(std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
            VkAccelerationStructureGeometryKHR geometry, u32 maxPrimitiveCounts, VkBuildAccelerationStructureFlagsKHR flags);

        void Build(CmdList& cmdList, const List<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags);
        void Update(CmdList& cmdList, const List<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags);
    };
}