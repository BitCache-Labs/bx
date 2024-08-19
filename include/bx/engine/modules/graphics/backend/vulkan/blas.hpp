#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/acceleration_structure.hpp"

namespace Vk
{
	class CmdList;
    class Buffer;
    class Device;
    class PhysicalDevice;
    class Mesh;

    class Blas : public AccelerationStructure {
    public:
        Blas(std::shared_ptr<Device> device, const PhysicalDevice& physicalDevice,
             CmdList& uploadCmdList, const Mesh& mesh);
        ~Blas();

    private:
        std::shared_ptr<Buffer> blas;
    };
}