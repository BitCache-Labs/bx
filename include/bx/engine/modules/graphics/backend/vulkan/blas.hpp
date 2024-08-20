#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/acceleration_structure.hpp"

namespace Vk
{
	class CmdList;
    class Buffer;
    class Device;
    class PhysicalDevice;

    struct BlasInfo
    {
        VkFormat vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        u32 vertexCount = 0;
        u32 vertexOffset = 0;
        u32 vertexStride = 0;
        VkIndexType indexType = VK_INDEX_TYPE_UINT32;
        u32 indexCount = 0;
        u32 indexOffset = 0;
    };

    class Blas : public AccelerationStructure {
    public:
        Blas(const String& name, std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
             CmdList& uploadCmdList, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, const BlasInfo& info);
        ~Blas();

    private:
        std::shared_ptr<Buffer> blas;
    };
}