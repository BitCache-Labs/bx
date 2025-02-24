#include <editor/vulkan/graphics_vulkan.hpp>

BX_EDITOR_MENUITEM_REGISTRATION(ICON_EDITOR"/Modules/Graphics", GraphicsVulkanEditor)
{
	Editor::Get().AddWindow<GraphicsVulkanEditor>();
}

GraphicsVulkanEditor::GraphicsVulkanEditor()
{
	SetTitle("Vulkan");
	SetExclusive(true);
	SetPresistent(false);
}

void GraphicsVulkanEditor::OnGui()
{
}