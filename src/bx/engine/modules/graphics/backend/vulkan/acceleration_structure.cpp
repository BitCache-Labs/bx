#include "bx/engine/modules/graphics/backend/vulkan/acceleration_structure.hpp"

#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/math.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/cmd_list.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/buffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/pfn.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

struct ScratchBuffer : public LazyInit<ScratchBuffer, std::shared_ptr<Vk::Buffer>>
{
    ScratchBuffer(std::shared_ptr<Vk::Device> device, const Vk::PhysicalDevice& physicalDevice)
    {
        data = std::shared_ptr<Vk::Buffer>(new Vk::Buffer("Scratch Buffer", device, physicalDevice,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 1024 * 1024 * 256, Vk::BufferLocation::GPU_ONLY));
    }
};

template<>
std::unique_ptr<ScratchBuffer> LazyInit<ScratchBuffer, std::shared_ptr<Vk::Buffer>>::cache = nullptr;

namespace Vk
{
    VkAccelerationStructureBuildSizesInfoKHR CalculateBuildSizes(const Device& device, const RTProperties& properties, VkAccelerationStructureBuildGeometryInfoKHR geometryInfo, u32 maxPrimitiveCounts)
    {
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        const u32* pMaxPrimitiveCounts = &maxPrimitiveCounts;
        Pfn::vkGetAccelerationStructureBuildSizesKHR(device.GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &geometryInfo, pMaxPrimitiveCounts, &sizeInfo);

        const uint64_t accelerationStructureAlignment = 256;
        const uint64_t scratchAlignment = properties.accelProperties.minAccelerationStructureScratchOffsetAlignment;
        sizeInfo.accelerationStructureSize = Math::AlignUp(sizeInfo.accelerationStructureSize, accelerationStructureAlignment);
        sizeInfo.buildScratchSize = Math::AlignUp(sizeInfo.buildScratchSize, scratchAlignment);

        return sizeInfo;
    }

	AccelerationStructure::AccelerationStructure(const String& name, std::shared_ptr<Device> device,
        const PhysicalDevice& physicalDevice, u32 size, VkAccelerationStructureTypeKHR type)
        : device(device), physicalDevice(physicalDevice)
    {
        buffer = std::shared_ptr<Buffer>(new Buffer(name, device, physicalDevice, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, size, BufferLocation::GPU_ONLY));

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.type = type;
        createInfo.size = size;
        createInfo.buffer = buffer->GetBuffer();
        createInfo.offset = 0;

        VK_ASSERT(!Pfn::vkCreateAccelerationStructureKHR(device->GetDevice(), &createInfo, nullptr, &this->accelerationStructure), "Failed to create acceleration structure.");
    }

    AccelerationStructure::~AccelerationStructure()
    {
        Pfn::vkDestroyAccelerationStructureKHR(this->device->GetDevice(),
                                               this->accelerationStructure, nullptr);
    }

    VkAccelerationStructureKHR AccelerationStructure::GetAccelerationStructure() const
    {
        return accelerationStructure;
    }

    std::shared_ptr<Buffer> AccelerationStructure::GetBuffer() const
    {
        return buffer;
    }

    Blas::Blas(const String& name, std::shared_ptr<Device> device,
        const PhysicalDevice& physicalDevice, u32 size)
        : AccelerationStructure(name, device, physicalDevice, size, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
    {

    }

    u32 Blas::RequiredSize(std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
        VkAccelerationStructureGeometryKHR geometry, u32 maxPrimitiveCounts, VkBuildAccelerationStructureFlagsKHR flags)
    {
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.flags = flags;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        return CalculateBuildSizes(*device, physicalDevice.RayTracingProperties(), buildInfo, maxPrimitiveCounts).accelerationStructureSize;
    }

    void Blas::Build(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags)
    {
        std::shared_ptr<Buffer> scratchBuffer = ScratchBuffer::Get(device, physicalDevice);

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.flags = flags;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        cmdList.BuildAccelerationStructure(buildInfo, rangeInfo, scratchBuffer, GetBuffer(), GetAccelerationStructure());
    }

    void Blas::Update(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags)
    {
        std::shared_ptr<Buffer> scratchBuffer = ScratchBuffer::Get(device, physicalDevice);

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.flags = flags;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        cmdList.UpdateAccelerationStructure(buildInfo, rangeInfo, scratchBuffer, GetBuffer(), GetAccelerationStructure());
    }

    Tlas::Tlas(const String& name, std::shared_ptr<Device> device,
        const PhysicalDevice& physicalDevice, u32 size)
        : AccelerationStructure(name, device, physicalDevice, size, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
    {

    }

    u32 Tlas::RequiredSize(std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
        VkAccelerationStructureGeometryKHR geometry, u32 maxPrimitiveCounts, VkBuildAccelerationStructureFlagsKHR flags)
    {
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.flags = flags;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        return CalculateBuildSizes(*device, physicalDevice.RayTracingProperties(), buildInfo, maxPrimitiveCounts).accelerationStructureSize;
    }

    void Tlas::Build(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags)
    {
        std::shared_ptr<Buffer> scratchBuffer = ScratchBuffer::Get(device, physicalDevice);

        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmdList.GetCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.flags = flags;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        cmdList.BuildAccelerationStructure(buildInfo, rangeInfo, scratchBuffer, GetBuffer(), GetAccelerationStructure());
    }

    void Tlas::Update(CmdList& cmdList, VkAccelerationStructureGeometryKHR geometry, VkAccelerationStructureBuildRangeInfoKHR rangeInfo, VkBuildAccelerationStructureFlagsKHR flags)
    {

    }
}