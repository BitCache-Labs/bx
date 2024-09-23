#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/modules/graphics/type.hpp"

#include "vulkan_api.hpp"

namespace Vk
{
	class Device;

    class Shader : NoCopy {
    public:
        Shader(const String& name, std::shared_ptr<Device> device, VkShaderStageFlagBits stage, const String& src, const List<ShaderIncludeRange>& includeRanges);
        ~Shader();

        VkShaderModule GetShader() const;

    private:
        VkShaderModule shader;

        const std::shared_ptr<Device> device;
    };
}