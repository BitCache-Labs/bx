#pragma once

struct AppConfig
{
	int argc = 0;
	char** argv = nullptr;
};

class Application
{
public:
	static int Launch(const AppConfig& config);

	static bool IsRunning();
	static void Close();

	static void Reload();

private:
	static bool Configure(const AppConfig& config);
	static void Tick();
};