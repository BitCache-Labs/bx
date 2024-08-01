#include "bx/engine/modules/graphics/backend/opengl/graphics_pipeline.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/opengl/validation.hpp"
#include "bx/engine/modules/graphics/backend/opengl/conversion.hpp"

namespace Gl
{
    GraphicsPipeline::GraphicsPipeline(ShaderProgram&& shaderProgram, const List<VertexBufferLayout>& vertexBuffers, const PipelineLayoutDescriptor& layout)
        : shaderProgram(std::move(shaderProgram))
    {
        glGenVertexArrays(1, &vao);

        glBindVertexArray(vao);
        for (u32 i = 0; i < vertexBuffers.size(); i++)
        {
            auto& layout = vertexBuffers[i];

            for (u32 j = 0; j < layout.attributes.size(); j++)
            {
                auto& attribute = layout.attributes[j];
                
                if (IsVertexFormatInt(attribute.format))
                    glVertexArrayAttribIFormat(vao, attribute.location, VertexFormatToGlSize(attribute.format), VertexFormatToGlType(attribute.format), attribute.offset);
                else
                    glVertexArrayAttribFormat(vao, attribute.location, VertexFormatToGlSize(attribute.format), VertexFormatToGlType(attribute.format), GL_FALSE, attribute.offset);
                glEnableVertexArrayAttrib(vao, attribute.location);
                glVertexArrayAttribBinding(vao, attribute.location, i);
            }
        }
        glBindVertexArray(0);
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        glDeleteVertexArrays(1, &vao);
    }

    u32 GraphicsPipeline::GetVaoHandle() const
    {
        return vao;
    }

    u32 GraphicsPipeline::GetShaderProgramHandle() const
    {
        return shaderProgram.GetHandle();
    }
}