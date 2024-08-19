#include "bx/engine/modules/graphics/backend/vulkan/acceleration_structure.hpp"

#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/math.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/buffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/pfn.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
	AccelerationStructure::AccelerationStructure(const String& name,
                                                 std::shared_ptr<Device> device,
                                                 const PhysicalDevice& physicalDevice)
        : device(device),
          rtProperties(physicalDevice.RayTracingProperties()),
          buildGeometryInfo{},
          buildSizesInfo{} {
    }

    AccelerationStructure::~AccelerationStructure() {
        // TODO: vma
        Pfn::vkDestroyAccelerationStructureKHR(this->device->GetDevice(),
                                               this->accelerationStructure, nullptr);
    }

    VkAccelerationStructureKHR AccelerationStructure::GetAccelerationStructure() const {
        return this->accelerationStructure;
    }

    const VkAccelerationStructureBuildSizesInfoKHR AccelerationStructure::GetBuildSizes() const {
        return this->buildSizesInfo;
    }

    VkAccelerationStructureBuildSizesInfoKHR AccelerationStructure::CalculateBuildSizes(
        uint32_t maxPrimitiveCounts) const {
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        const uint32_t* pMaxPrimitiveCounts = &maxPrimitiveCounts;

        Pfn::vkGetAccelerationStructureBuildSizesKHR(
            this->device->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &this->buildGeometryInfo, pMaxPrimitiveCounts, &sizeInfo);

        const uint64_t accelerationStructureAlignment = 256;
        const uint64_t scratchAlignment =
            this->rtProperties.accelProperties.minAccelerationStructureScratchOffsetAlignment;
        
        sizeInfo.accelerationStructureSize =
            Math::AlignUp(sizeInfo.accelerationStructureSize, accelerationStructureAlignment);
        sizeInfo.buildScratchSize = Math::AlignUp(sizeInfo.buildScratchSize, scratchAlignment);

        return sizeInfo;
    }

    void AccelerationStructure::CreateAccelerationStructure(Buffer& resultBuffer,
                                                            VkDeviceSize resultOffset) {
        VkAccelerationStructureCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.type = this->buildGeometryInfo.type;
        createInfo.size = GetBuildSizes().accelerationStructureSize;
        createInfo.buffer = resultBuffer.GetBuffer();
        createInfo.offset = resultOffset;

        VK_ASSERT(!Pfn::vkCreateAccelerationStructureKHR(this->device->GetDevice(), &createInfo,
                                                          nullptr, &this->accelerationStructure),
                   "Failed to create acceleration structure.");
    }
}