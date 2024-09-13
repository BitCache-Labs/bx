#pragma once

class Application
{
private:
	friend class Runtime;

	static bool Initialize();
	static void Shutdown();
};