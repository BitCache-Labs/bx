#include "bx/core/plugin.hpp"

#include "bx/core/byte_types.hpp"
#include "bx/containers/list.hpp"

#include <rttr/registration>
using namespace rttr;

RTTR_REGISTRATION
{
	rttr::registration::class_<Plugin2>("Plugin")
	//.constructor<>()
	//.constructor<int>()
	//.constructor<int, int>()
	//.property("hp", &player::get_hp, &player::set_hp)
	//.property("speed", &player::speed)
	//.property_readonly("bullets", &player::bullets)
	.method("Initialize", &Plugin2::Initialize)
	.method("Shutdown", &Plugin2::Shutdown)
	;
}

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