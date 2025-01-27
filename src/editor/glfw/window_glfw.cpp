#include <editor/glfw/window_glfw.hpp>
#include <engine/glfw/window_glfw.hpp>

EDITOR_MENUITEM("Modules/Window/GLFW", WindowGLFWEditor)
void WindowGLFWEditor::ShowWindow()
{
	Editor::Get().AddWindow<WindowGLFWEditor>();
}

WindowGLFWEditor::WindowGLFWEditor()
{
	SetTitle("GLFW");
	SetExclusive(true);
	SetPresistent(false);
}

void WindowGLFWEditor::OnGui()
{
	auto& impl = WindowGLFW::Get();
	const char* glfwVersion = glfwGetVersionString();

	CString<64> fmtStr{};
	fmtStr.format("GLFW version: {}", glfwVersion);
	ImGui::Text(fmtStr.c_str());
}