#include <editor/editor.hpp>

class GraphicsVulkanEditor final : public EditorView
{
public:
	GraphicsVulkanEditor();
	void OnGui() override;
};

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