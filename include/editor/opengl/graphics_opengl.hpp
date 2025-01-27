#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API GraphicsOpenGLEditor final
	: public EditorWindow
{
	BX_TYPE(GraphicsOpenGLEditor, EditorWindow)

public:
	static void ShowWindow();

public:
	GraphicsOpenGLEditor();
	void OnGui() override;
};
