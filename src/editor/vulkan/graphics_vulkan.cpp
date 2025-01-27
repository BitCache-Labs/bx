#include <editor/vulkan/graphics_vulkan.hpp>

EDITOR_MENU("Modules/Graphics/Vulkan", []() { EditorManager::Get().AddView(meta::make_unique<GraphicsVulkanEditor>()); })

GraphicsVulkanEditor::GraphicsVulkanEditor()
{
	SetTitle("Vulkan");
	SetExclusive(true);
	SetPresistent(false);
}

void GraphicsVulkanEditor::OnGui()
{
}