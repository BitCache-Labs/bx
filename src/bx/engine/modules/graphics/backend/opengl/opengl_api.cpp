#include "bx/engine/modules/graphics/backend/opengl/opengl_api.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/opengl/validation.hpp"

namespace Gl
{
	b8 Init(b8 debug)
	{
#ifdef BX_WINDOW_GLFW_BACKEND
#ifdef BX_GRAPHICS_OPENGL_BACKEND
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            BX_LOGE("Failed to initialize GLAD GL.");
            return false;
        }
#elif defined BX_GRAPHICS_OPENGLES_BACKEND
        if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress))
        {
            BX_LOGE("Failed to initialize GLAD GLES.");
            return false;
        }
#endif // BX_GRAPHICS_OPENGLES_BACKEND
#endif // BX_WINDOW_GLFW_BACKEND

        if (debug)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(GlDebugCallback, nullptr);
        }

        return true;
	}
}