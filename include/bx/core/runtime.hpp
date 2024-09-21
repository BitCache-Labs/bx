#pragma once

class Runtime
{
public:
	static int Launch(int argc, char** argv);

	static bool IsRunning();
	static void Close();

	static void Reload();
};