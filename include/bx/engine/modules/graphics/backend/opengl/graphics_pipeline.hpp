#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/modules/graphics/type.hpp"

#include "opengl_api.hpp"

#include "bx/engine/modules/graphics/backend/opengl/shader.hpp"

namespace Gl
{
    class GraphicsPipeline : NoCopy
    {
    public:
        GraphicsPipeline(ShaderProgram&& shaderProgram, const List<VertexBufferLayout>& vertexBuffers, const PipelineLayoutDescriptor& layout);
        ~GraphicsPipeline();

        u32 GetVaoHandle() const;
        u32 GetShaderProgramHandle() const;

    private:
        u32 vao;
        ShaderProgram shaderProgram;
    };
}