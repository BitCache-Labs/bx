#pragma once

#include <bx/bx.hpp>

struct AppConfig
{
	AppConfig(int argc, char** argv)
		: argc(argc)
		, argv(argv)
	{}

	int argc = 0;
	char** argv = nullptr;
};

class BX_API Application
{
public:
	static int Launch(const AppConfig& config);

	static bool IsRunning();
	static void Close();

	static void Reload();
	static void Tick();
};