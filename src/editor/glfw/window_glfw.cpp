#include <editor/editor.hpp>

class WindowGLFWEditor final : public EditorView
{
public:
	WindowGLFWEditor();
	void OnGui() override;
};

EDITOR_MENU("Modules/Window/GLFW", []() { EditorManager::Get().AddView(meta::make_unique<WindowGLFWEditor>()); })

WindowGLFWEditor::WindowGLFWEditor()
{
	SetTitle("GLFW");
	SetExclusive(true);
	SetPresistent(false);
}

void WindowGLFWEditor::OnGui()
{
}