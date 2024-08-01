#pragma once

#include "bx/engine/core/type.hpp"

#include <glad/glad.h>

#ifdef BX_WINDOW_GLFW_BACKEND
#include <GLFW/glfw3.h>
#endif // BX_WINDOW_GLFW_BACKEND

namespace Gl
{
	b8 Init(b8 debug);
}