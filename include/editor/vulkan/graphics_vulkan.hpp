#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API GraphicsVulkanEditor final
	: public EditorView
{
	BX_TYPE(GraphicsVulkanEditor, EditorWindow)

public:
	GraphicsVulkanEditor();
	void OnGui() override;
};