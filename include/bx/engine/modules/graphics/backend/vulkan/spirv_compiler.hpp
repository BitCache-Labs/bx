#pragma once

#include "bx/engine/core/type.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/modules/graphics/type.hpp"

#include "vulkan_api.hpp"

#include <glslang/Include/Types.h>

namespace Vk
{
    class SpirVCompiler
    {
    public:
        static SpirVCompiler& Instance();
        ~SpirVCompiler();

        List<u32> Compile(const String& name, EShLanguage stage, const String& src, const List<ShaderIncludeRange>& includeRanges);

    private:
        SpirVCompiler();
    };
}