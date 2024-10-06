#pragma once

#define VULKAN_VERSION VK_API_VERSION_1_3
#define VMA_VULKAN_VERSION 1003000

#ifdef BX_WINDOW_GLFW_BACKEND
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // BX_WINDOW_GLFW_BACKEND

#include <vulkan/vulkan.hpp>

#ifdef _DEBUG
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif
#include <vk_mem_alloc.h>
