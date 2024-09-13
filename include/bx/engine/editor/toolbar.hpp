#pragma once

class Toolbar
{
public:
	static void Initialize();
	static void Shutdown();

	static void Present();

	static void Reset();
	static bool IsPlaying();
	static bool IsPaused();
	static bool ConsumeNextFrame();
};