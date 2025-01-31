#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API WindowGLFWEditor final
	: public EditorWindow
{
	BX_TYPE(WindowGLFWEditor, EditorWindow)

public:
	WindowGLFWEditor();
	void OnGui() override;
};
