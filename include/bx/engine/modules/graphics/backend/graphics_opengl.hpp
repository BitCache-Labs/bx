#pragma once

#include "bx/engine/modules/graphics.hpp"

#include "opengl/opengl_api.hpp"

class GraphicsOpenGL
{
public:
    static GLuint GetRawBufferHandle(BufferHandle buffer);
    static GLuint GetRawTextureHandle(TextureHandle texture);
};