#include "bx/core/plugin.hpp"

#include "bx/core/byte_types.hpp"
#include "bx/containers/list.hpp"

static List<Plugin> g_plugins;

void PluginManager::RegisterPlugin(const Plugin& plugin)
{
	g_plugins.emplace_back(plugin);
}

bool PluginManager::Initialize()
{
	for (SizeType i = 0; i < g_plugins.size(); ++i)
	{
		const auto& plugin = g_plugins[i];
		if (!plugin.Initialize())
			return false;
	}
	return true;
}

void PluginManager::Shutdown()
{
	for (SizeType i = g_plugins.size(); i-- > 0;)
	{
		const auto& plugin = g_plugins[i];
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