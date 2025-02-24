#include <editor/opengl/graphics_opengl.hpp>

BX_EDITOR_MENUITEM_REGISTRATION(ICON_EDITOR"/Modules/Graphics", GraphicsOpenGLEditor)
{
	Editor::Get().AddWindow<GraphicsOpenGLEditor>();
}

GraphicsOpenGLEditor::GraphicsOpenGLEditor()
{
	SetTitle("OpenGL");
	SetExclusive(true);
	SetPresistent(false);
}

void GraphicsOpenGLEditor::OnGui(EditorApplication& app)
{
}