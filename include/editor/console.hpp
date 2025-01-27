#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class Console final
	: public EditorWindow
{
	BX_TYPE(Console, EditorWindow)

public:
	Console();
	void OnGui() override;
};