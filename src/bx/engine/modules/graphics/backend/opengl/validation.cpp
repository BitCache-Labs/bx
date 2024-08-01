#include "bx/engine/modules/graphics/backend/opengl/validation.hpp"

#include "bx/engine/core/macros.hpp"

namespace Gl
{
    void GlDebugCallback(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message,
        const void*)
    {
        if (severity == GL_DEBUG_SEVERITY_HIGH)
            BX_FAIL("GL: {}", message);
    }

    void DebugNames::Set(GLenum identifier, GLuint object, const std::string& name)
    {
        GLint maxLabelLength;
        glGetIntegerv(GL_MAX_LABEL_LENGTH, &maxLabelLength);
        BX_ASSERT(name.size() < maxLabelLength, "Debug name too long.");

        glObjectLabel(identifier, object, static_cast<GLsizei>(name.size()), name.c_str());
    }
}