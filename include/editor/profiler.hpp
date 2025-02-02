#pragma once

#include <engine/api.hpp>
#include <engine/byte_types.hpp>
#include <editor/editor.hpp>

class BX_API ProfilerEditor final
	: public EditorWindow
{
	BX_TYPE(ProfilerEditor, EditorWindow)

public:
	ProfilerEditor();
	void OnGui(EditorApplication& app) override;

private:
	i32 m_frames = 0;
	f32 m_time = 0;
	i32 m_fps = 0;

	i32 m_frame = 0;
};