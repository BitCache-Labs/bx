#pragma once

#include "bx/editor/view.hpp"

class Toolbar final : public View
{
public:
	//static void AddView(View* view);

	bool Initialize() override;
	void Shutdown() override;

	void OnReload() override;
	void OnPresent() override;

	//void Reset();
	//bool IsPlaying();
	//bool IsPaused();
	//bool ConsumeNextFrame();
};