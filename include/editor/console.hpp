#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API ConsoleEditor final
	: public EditorWindow
{
	BX_TYPE(ConsoleEditor, EditorWindow)

public:
	ConsoleEditor();
	void OnGui(EditorApplication& app) override;
};