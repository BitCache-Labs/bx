#pragma once

#include <rttr/rttr_enable.h>

class Plugin2
{
	RTTR_ENABLE()

public:
	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;
};

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