#pragma once

struct Plugin
{
	using InitializeFn = bool(*)();
	using ShutdownFn = void(*)();
	using ReloadFn = void(*)();

	InitializeFn Initialize = nullptr;
	ShutdownFn Shutdown = nullptr;
	ReloadFn Reload = nullptr;
};

class PluginManager
{
public:
	static void RegisterPlugin(const Plugin& plugin);

private:
	friend class Application;

	static bool Initialize();
	static void Shutdown();
	static void Reload();
};