#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API AssetsEditor final
	: public EditorWindow
{
	BX_TYPE(AssetsEditor, EditorWindow)

public:
	AssetsEditor();
	void OnGui(EditorApplication& app) override;
};