#pragma once

#include "bx/engine/modules/graphics.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set.hpp"

#include "vulkan/vulkan_api.hpp"

#include <memory>

struct ImGui_ImplVulkan_InitInfo;

class GraphicsVulkan
{
public:
    static ImGui_ImplVulkan_InitInfo ImGuiInitInfo();
    static std::shared_ptr<Vk::DescriptorSet> TextureAsDescriptorSet(TextureViewHandle textureView);
    static u32 GetCurrentFrameIdx();

    static VkCommandBuffer RawCommandBuffer();
    static void WaitIdle();
};