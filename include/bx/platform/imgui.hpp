#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

class ImGuiImpl
{
public:
	static bool Initialize();
	static void Reload();
	static void Shutdown();

	static void NewFrame();
	static void EndFrame();

private:
	static bool Initialize_Window();
	static void Shutdown_Window();
	static void NewFrame_Window();
	static void EndFrame_Window();

	static bool Initialize_Graphics();
	static void Shutdown_Graphics();
	static void NewFrame_Graphics();
	static void EndFrame_Graphics();
};