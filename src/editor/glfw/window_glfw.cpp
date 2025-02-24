#include <editor/glfw/window_glfw.hpp>
#include <engine/glfw/window_glfw.hpp>

BX_EDITOR_MENUITEM_REGISTRATION(ICON_EDITOR"/Modules/Window", WindowGLFWEditor)
{
	Editor::Get().AddWindow<WindowGLFWEditor>();
}

WindowGLFWEditor::WindowGLFWEditor()
{
	SetTitle("GLFW");
	SetExclusive(true);
	SetPresistent(false);
}

void WindowGLFWEditor::OnGui(EditorApplication& app)
{
	auto& impl = WindowGLFW::Get();
	const char* glfwVersion = glfwGetVersionString();

	CString<64> fmtStr{};
	fmtStr.format("GLFW version: {}", glfwVersion);
	ImGui::Text(fmtStr.c_str());
}