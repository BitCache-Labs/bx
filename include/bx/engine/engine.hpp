#pragma once

#include <bx/core/plugin.hpp>

class Engine2 : public Plugin2
{
	RTTR_ENABLE(Plugin2)

public:
	bool Initialize() override;
	void Shutdown() override;
	//static void Reload();
};

class Engine
{
public:
	static Plugin GetPlugin()
	{
		Plugin plugin;
		plugin.Initialize = &Initialize;
		plugin.Shutdown = &Shutdown;
		plugin.Reload = &Reload;
		return plugin;
	}

private:
	static bool Initialize();
	static void Shutdown();
	static void Reload();
};