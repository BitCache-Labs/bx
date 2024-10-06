#pragma once

#include "bx/engine/modules/graphics.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/cmd_list.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set.hpp"

#include "vulkan/vulkan_api.hpp"

#include <memory>

struct ImGui_ImplVulkan_InitInfo;

class GraphicsVulkan
{
public:
    static std::shared_ptr<Vk::Device> GetDevice();
    static const Vk::PhysicalDevice& GetPhysicalDevice();
    static std::shared_ptr<Vk::CmdList> GetCurrentCommandList();

    static std::shared_ptr<Vk::Image> GetImage(TextureHandle texture);
    static std::shared_ptr<Vk::ImageView> GetImageView(TextureViewHandle textureView);

    static ImGui_ImplVulkan_InitInfo ImGuiInitInfo();
    static std::shared_ptr<Vk::DescriptorSet> TextureAsDescriptorSet(TextureViewHandle textureView);
    static u32 GetCurrentFrameIdx();
};