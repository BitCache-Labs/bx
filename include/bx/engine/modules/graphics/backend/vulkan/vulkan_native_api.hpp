#pragma once

#include "vulkan_api.hpp"

#ifdef BX_PLATFORM_PC
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <vulkan/vulkan_win32.h>
#elif defined BX_PLATFORM_LINUX
// TODO
#endif // BX_PLATFORM_PC

#ifdef BX_WINDOW_GLFW_BACKEND
#ifdef BX_PLATFORM_PC
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined BX_PLATFORM_LINUX
// TODO
#endif // BX_PLATFORM_PC
#include <GLFW/glfw3native.h>
#endif // BX_WINDOW_GLFW_BACKEND