#include <editor/opengl/graphics_opengl.hpp>

EDITOR_MENUITEM("Modules/Graphics/OpenGL", GraphicsOpenGLEditor)
void GraphicsOpenGLEditor::ShowWindow()
{
	Editor::Get().AddWindow<GraphicsOpenGLEditor>();
}

GraphicsOpenGLEditor::GraphicsOpenGLEditor()
{
	SetTitle("OpenGL");
	SetExclusive(true);
	SetPresistent(false);
}

void GraphicsOpenGLEditor::OnGui()
{
}