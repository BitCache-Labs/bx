#include <editor/editor.hpp>

class GraphicsVulkanEditor final : public EditorView
{
public:
	GraphicsVulkanEditor();
	void OnGui() override;
};