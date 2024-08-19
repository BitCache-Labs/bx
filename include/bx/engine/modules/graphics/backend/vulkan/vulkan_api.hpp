#pragma once

#define VULKAN_VERSION VK_API_VERSION_1_2
#define VMA_VULKAN_VERSION 1002000

#ifdef BX_WINDOW_GLFW_BACKEND
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // BX_WINDOW_GLFW_BACKEND

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
