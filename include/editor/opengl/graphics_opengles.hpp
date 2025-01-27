#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API GraphicsOpenGLESEditor final
	: public EditorView
{
	BX_TYPE(GraphicsOpenGLESEditor, EditorWindow)

public:
	GraphicsOpenGLESEditor();
	void OnGui() override;
};