#include "bx/core/plugin.hpp"

#include "bx/containers/list.hpp"

static List<Plugin> g_plugins;

void PluginManager::RegisterPlugin(const Plugin& plugin)
{
	g_plugins.emplace_back(plugin);
}

bool PluginManager::Initialize()
{
	for (const auto& plugin : g_plugins)
	{
		if (!plugin.Initialize())
			return false;
	}
	return true;
}

void PluginManager::Shutdown()
{
	for (const auto& plugin : g_plugins)
	{
		plugin.Shutdown();
	}
}

void PluginManager::Reload()
{
	for (const auto& plugin : g_plugins)
	{
		plugin.Reload();
	}
}

void PluginManager::Tick()
{
	for (const auto& plugin : g_plugins)
	{
		plugin.Tick();
	}
}