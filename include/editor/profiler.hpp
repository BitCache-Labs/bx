#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API ProfilerEditor final
	: public EditorWindow
{
	BX_TYPE(ProfilerEditor, EditorWindow)

public:
	bool Initialize() override;
	void Reload() override;
	void Shutdown() override;

	const char* GetTitle() const override;
	void Present(const char* title, bool& isOpen) override;

private:
	int m_frames = 0;
	float m_time = 0;
	int m_fps = 0;

	int m_frame = 0;
};