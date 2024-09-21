#pragma once

struct Plugin
{
	using InitializeFn = bool(*)();
	using ShutdownFn = void(*)();
	using ReloadFn = void(*)();
	using TickFn = void(*)();

	InitializeFn Initialize = nullptr;
	ShutdownFn Shutdown = nullptr;
	ReloadFn Reload = nullptr;
	TickFn Tick = nullptr;
};

class PluginManager
{
public:
	static void RegisterPlugin(const Plugin& plugin);

private:
	friend class Runtime;

	static bool Initialize();
	static void Shutdown();
	static void Reload();
	static void Tick();
};