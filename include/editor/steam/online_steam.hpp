#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API OnlineSteamEditor final
	: public EditorWindow
{
	BX_TYPE(OnlineSteamEditor, EditorWindow)

public:
	OnlineSteamEditor();
	void OnGui(EditorApplication& app) override;
};