#pragma once

#include "bx/engine/core/type.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/list.hpp"

#include "vulkan_api.hpp"

#include <glslang/Include/glslang_c_shader_types.h>

namespace Vk
{
    class SpirVCompiler
    {
    public:
        static SpirVCompiler& Instance();
        ~SpirVCompiler();

        List<u32> Compile(const String& name, glslang_stage_t stage, const String& src);

    private:
        SpirVCompiler();
    };
}