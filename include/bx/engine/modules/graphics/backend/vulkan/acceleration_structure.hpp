
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
        AccelerationStructure(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, u32 size, VkAccelerationStructureTypeKHR type);
        ~AccelerationStructure();

        VkAccelerationStructureKHR GetAccelerationStructure() const;
        std::shared_ptr<Buffer> GetBuffer() const;

    private:
        const std::shared_ptr<Device> device;
        const PhysicalDevice& physicalDevice;

        VkAccelerationStructureKHR accelerationStructure;
        std::shared_ptr<Buffer> buffer;
    };

    class Blas
    {
    public:
        Blas(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, u32 size);

        static u32 RequiredSize(std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
            VkAccelerationStructureGeometryKHR geometry, u32 maxPrimitiveCounts, VkBuildAccelerationStructureFlagsKHR flags);

        void Build(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags);
        void Update(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags);

        std::shared_ptr<AccelerationStructure> GetAccelerationStructure() const;

    private:
        const std::shared_ptr<Device> device;
        const PhysicalDevice& physicalDevice;

        std::shared_ptr<AccelerationStructure> accelerationStructure = nullptr;
    };

    class Tlas
    {
    public:
        Tlas(const String& name, std::shared_ptr<Device> device,
            const PhysicalDevice& physicalDevice, u32 size);

        static u32 RequiredSize(std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
            VkAccelerationStructureGeometryKHR geometry, u32 maxPrimitiveCounts, VkBuildAccelerationStructureFlagsKHR flags);

        void Build(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags);
        void Update(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags);

        std::shared_ptr<AccelerationStructure> GetAccelerationStructure() const;

        void TrackBlas(std::shared_ptr<Blas> blas);

    private:
        const std::shared_ptr<Device> device;
        const PhysicalDevice& physicalDevice;

        std::shared_ptr<AccelerationStructure> accelerationStructure;

        std::vector<std::shared_ptr<Blas>> trackedBlases;
    };
}