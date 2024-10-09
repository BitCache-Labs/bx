#pragma once

#include "bx/editor/view.hpp"

class ProfilerView final : public View
{
public:
	bool Initialize() override;
	void Shutdown() override;

	void OnReload() override;
	void OnPresent() override;

private:
	bool m_open = true;

	int m_frames = 0;
	float m_time = 0;
	int m_fps = 0;

	int m_frame = 0;
};