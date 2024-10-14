#pragma once

#include "bx/editor/view.hpp"

class ProfilerView final : public View
{
public:
	bool Initialize() override;
	void Reload() override;
	void Shutdown() override;

	void Present() override;

private:
	int m_frames = 0;
	float m_time = 0;
	int m_fps = 0;

	int m_frame = 0;
};