#pragma once

class ImGuiImpl
{
public:
	static void NewFrame();
	static void EndFrame();

private:
	friend class Runtime;
	friend class Module;

	static bool Initialize();
	static void Reload();
	static void Shutdown();
};