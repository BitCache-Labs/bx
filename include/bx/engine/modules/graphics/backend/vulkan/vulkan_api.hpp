#pragma once

#define VULKAN_VERSION VK_API_VERSION_1_2
#define VMA_VULKAN_VERSION 1002000

// TODO: rename platform pc to platform windows?
#ifdef BX_PLATFORM_PC
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined BX_PLATFORM_LINUX
// TODO
#endif // BX_PLATFORM_PC

#ifdef BX_WINDOW_GLFW_BACKEND
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef BX_PLATFORM_PC
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined BX_PLATFORM_LINUX
// TODO
#endif // BX_PLATFORM_PC
#include <GLFW/glfw3native.h>
#endif // BX_WINDOW_GLFW_BACKEND

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
