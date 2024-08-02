#include "bx/engine/modules/graphics/backend/vulkan/shader.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/spirv_compiler.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

namespace Vk
{
    glslang_stage_t GlslangStageFromVkStage(VkShaderStageFlagBits stage)
    {
        switch (stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return GLSLANG_STAGE_VERTEX;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return GLSLANG_STAGE_GEOMETRY;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return GLSLANG_STAGE_FRAGMENT;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return GLSLANG_STAGE_COMPUTE;
        default:
            BX_FAIL("Unsupported shader stage.");
        }
    }

    Shader::Shader(const String& name, std::shared_ptr<Device> device, VkShaderStageFlagBits stage, const String& src)
        : device(device) {
        List<u32> code = SpirVCompiler::Instance().Compile(name, GlslangStageFromVkStage(stage), src);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size() * 4;
        createInfo.pCode = reinterpret_cast<const u32*>(code.data());

        BX_ASSERT(!vkCreateShaderModule(device->GetDevice(), &createInfo, nullptr, &this->shader),
            "Failed to create shader.");
        DebugNames::Set(*device, VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE,
            reinterpret_cast<uint64_t>(this->shader), name);
    }

    Shader::~Shader() {
        vkDestroyShaderModule(this->device->GetDevice(), this->shader, nullptr);
    }

    VkShaderModule Shader::GetShader() const {
        return this->shader;
    }
}