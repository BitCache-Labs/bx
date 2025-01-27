#include <editor/opengl/graphics_opengles.hpp>

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