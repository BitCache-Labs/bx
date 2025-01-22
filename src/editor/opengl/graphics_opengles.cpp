#include <editor/editor.hpp>

class GraphicsOpenGLESEditor final : public EditorView
{
public:
	GraphicsOpenGLESEditor();
	void OnGui() override;
};

EDITOR_MENU("Modules/Graphics/OpenGLES", []() { EditorManager::Get().AddView(meta::make_unique<GraphicsOpenGLESEditor>()); })

GraphicsOpenGLESEditor::GraphicsOpenGLESEditor()
{
	SetTitle("OpenGLES");
	SetExclusive(true);
	SetPresistent(false);
}

void GraphicsOpenGLESEditor::OnGui()
{
}