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
	Blas::Blas(std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
               CmdList& uploadCmdList, const Mesh& mesh)
        : AccelerationStructure(mesh.name, device, physicalDevice) {
        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexData.deviceAddress =
            mesh.vertexBuffer->GetDeviceAddress();
        geometry.geometry.triangles.vertexStride = sizeof(Vertex);
        geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(mesh.vertexCount);
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.indexData.deviceAddress = mesh.indexBuffer->GetDeviceAddress();
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.transformData = {};

        this->buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        this->buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        this->buildGeometryInfo.geometryCount = 1;
        this->buildGeometryInfo.pGeometries = &geometry;
        this->buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        this->buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        this->buildGeometryInfo.srcAccelerationStructure = nullptr;

        this->buildSizesInfo = CalculateBuildSizes(mesh.indexCount / 3);

        std::shared_ptr<Buffer> scratchBuffer =
            std::make_shared<Buffer>("BLAS scratch buffer", device, physicalDevice,
                                     BufferUsageBits::STORAGE_BUFFER, 69, BufferLocation::GPU_ONLY);
        CreateAccelerationStructure(*this->blas, 0);

        uploadCmdList.BuildBLAS(geometry, scratchBuffer, this->blas, GetAccelerationStructure(),
                                mesh.indexCount);
    }

    Blas::~Blas() {
    }
}