#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "opengl_api.hpp"

namespace Gl
{
    void GlDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        const GLchar* message, const void* userParam);
    
    class DebugNames
    {
    public:
        static void Set(GLenum identifier, GLuint object, const String& name);
    };
}