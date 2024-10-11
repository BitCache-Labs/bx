#pragma once

#include <bx/bx.hpp>

//#include <bx/engine/imconfig.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

class BX_API ImGuiImpl
{
public:
	static bool Initialize();
	static void Reload();
	static void Shutdown();

	static void NewFrame();
	static void EndFrame();

	static ImGuiContext* GetCurrentContext();
};