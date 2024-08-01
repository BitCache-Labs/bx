#include "bx/engine/modules/graphics/backend/opengl/shader.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/opengl/validation.hpp"

namespace Gl
{
	Shader::Shader(const String& name, GLenum type, const String& src)
	{
        handle = glCreateShader(type);
        DebugNames::Set(GL_SHADER, handle, name);

        const char* pSourceCode = src.c_str();
        glShaderSource(handle, 1, &pSourceCode, NULL);
        glCompileShader(handle);

        GLint success;
        GLchar log[1024 * 4];
        glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(handle, sizeof(log), NULL, log);
            BX_LOGE("Failed to compile shader '{}'.\n---------------------------------\n{}",
                name.c_str(), log);
        }
	}

    Shader::Shader(Shader&& other) noexcept
        : handle(other.handle)
    {
        other.handle = UINT_MAX;
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        handle = other.handle;
        other.handle = UINT_MAX;
        return *this;
    }

    Shader::~Shader()
    {
        if (handle != UINT_MAX)
            glDeleteShader(handle);
    }

    ShaderProgram::ShaderProgram(const String& name, const List<Shader*>& shaders)
    {
        handle = glCreateProgram();
        DebugNames::Set(GL_PROGRAM, handle, name);

        for (auto& shader : shaders)
        {
            glAttachShader(handle, shader->handle);
        }
        glLinkProgram(handle);

        GLint success;
        GLchar log[1024 * 4];
        glGetProgramiv(handle, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(handle, sizeof(log), NULL, log);
            BX_LOGE("Failed to compile program '{}'.\n---------------------------------\n{}",
                name.c_str(), log);
        }
    }

    ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
        : handle(other.handle)
    {
        other.handle = UINT_MAX;
    }

    ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
    {
        handle = other.handle;
        other.handle = UINT_MAX;
        return *this;
    }

    ShaderProgram::~ShaderProgram()
    {
        if (handle != UINT_MAX)
            glDeleteProgram(handle);
    }

    u32 ShaderProgram::GetHandle() const
    {
        return handle;
    }
}