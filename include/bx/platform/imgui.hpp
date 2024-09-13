#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

class ImGuiImpl
{
public:
	static void NewFrame();
	static void EndFrame();

private:
	friend class Runtime;
	friend class Module;

	static bool Initialize();
	static void Reload();
	static void Shutdown();

	static bool Initialize_Window();
	static void Shutdown_Window();
	static void NewFrame_Window();
	static void EndFrame_Window();

	static bool Initialize_Graphics();
	static void Shutdown_Graphics();
	static void NewFrame_Graphics();
	static void EndFrame_Graphics();
};