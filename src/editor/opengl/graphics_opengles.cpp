#include <editor/opengl/graphics_opengles.hpp>

BX_EDITOR_MENUITEM_REGISTRATION(ICON_EDITOR"/Modules/Graphics", GraphicsOpenGLESEditor)
{
	Editor::Get().AddWindow<GraphicsOpenGLESEditor>();
}

GraphicsOpenGLESEditor::GraphicsOpenGLESEditor()
{
	SetTitle("OpenGLES");
	SetExclusive(true);
	SetPresistent(false);
}

void GraphicsOpenGLESEditor::OnGui()
{
}