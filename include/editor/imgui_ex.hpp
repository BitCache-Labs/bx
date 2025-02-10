#pragma once

#include <engine/api.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome5.h>

enum struct BX_API EditorTheme
{
	DARK,
	LIGHT,
	GRAY,
	ACRYLIC,
	ENUM_COUNT
};

namespace ImGui
{
	void Tooltip(const char* tooltip);

	void PushMenuBarTheme();
	void PopMenuBarTheme();

	void StyleEmbraceTheDarkness();
	void StyleFollowTheLight();
	void StyleGoGray();
	void StyleSeeThrough();
}