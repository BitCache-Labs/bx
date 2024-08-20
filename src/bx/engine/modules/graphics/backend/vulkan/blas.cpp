#include "bx/engine/modules/graphics/backend/vulkan/blas.hpp"

#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/math.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/buffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/cmd_list.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/pfn.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
	/*Blas::Blas(const String& name, std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
        CmdList& uploadCmdList, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, const BlasInfo& info)
        : AccelerationStructure(name, device, physicalDevice)
    {
        BX_ENSURE(info.vertexOffset == 0 && info.indexOffset == 0);

        VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexData.deviceAddress = vertexBuffer->GetDeviceAddress();
        triangles.vertexStride = info.vertexStride;
        triangles.maxVertex = info.vertexCount - 1;
        triangles.vertexFormat = info.vertexFormat;
        triangles.indexData.deviceAddress = indexBuffer->GetDeviceAddress();
        triangles.indexType = info.indexType;

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.triangles = triangles;

        VkAccelerationStructureBuildRangeInfoKHR offset{};
        offset.firstVertex = 0;
        offset.primitiveCount = info.indexCount / 3;
        offset.primitiveOffset = 0;
        offset.transformOffset = 0;

        this->buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        this->buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        this->buildGeometryInfo.geometryCount = 1;
        this->buildGeometryInfo.pGeometries = &geometry;
        this->buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        this->buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        this->buildSizesInfo = CalculateBuildSizes(info.indexCount / 3);

        this->blas = std::make_shared<Buffer>("BLAS buffer", device, physicalDevice,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, buildSizesInfo.accelerationStructureSize, BufferLocation::GPU_ONLY);
        std::shared_ptr<Buffer> scratchBuffer = std::make_shared<Buffer>("BLAS scratch buffer", device, physicalDevice,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buildSizesInfo.buildScratchSize, BufferLocation::GPU_ONLY);

        CreateAccelerationStructure(*this->blas, 0);

        uploadCmdList.BuildBLAS(geometry, scratchBuffer, this->blas, GetAccelerationStructure(),
                                info.indexCount);
    }

    Blas::~Blas() {
    }*/
}