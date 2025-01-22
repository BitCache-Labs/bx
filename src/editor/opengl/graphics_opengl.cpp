#include <editor/editor.hpp>

class GraphicsOpenGLEditor final : public EditorView
{
public:
	GraphicsOpenGLEditor();
	void OnGui() override;
};

EDITOR_MENU("Modules/Graphics/OpenGL", []() { EditorManager::Get().AddView(meta::make_unique<GraphicsOpenGLEditor>()); })

GraphicsOpenGLEditor::GraphicsOpenGLEditor()
{
	SetTitle("OpenGL");
	SetExclusive(true);
	SetPresistent(false);
}

void GraphicsOpenGLEditor::OnGui()
{
}