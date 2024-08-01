#pragma once

#include "bx/engine/modules/graphics.hpp"

#include "vulkan/vulkan_api.hpp"

struct ImGui_ImplVulkan_InitInfo;

class GraphicsVulkan
{
public:
    static ImGui_ImplVulkan_InitInfo ImGuiInitInfo();

    static VkCommandBuffer RawCommandBuffer();
    static void WaitIdle();
};