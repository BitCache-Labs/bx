#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API SettingsEditor final
	: public EditorWindow
{
	BX_TYPE(SettingsEditor, EditorWindow)

public:
	SettingsEditor();
	void OnGui(EditorApplication& app) override;
};