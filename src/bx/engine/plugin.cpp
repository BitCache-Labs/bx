#include "bx/engine/plugin.hpp"

#include <bx/core/module.hpp>
#include <bx/core/macros.hpp>
#include <bx/containers/list.hpp>

#include <iostream>
#include <rttr/type>
#include <rttr/registration>

RTTR_PLUGIN_REGISTRATION
{
    rttr::registration::class_<Plugin>("Plugin")
    .method("Initialize", &Plugin::Initialize)
    .method("Reload", &Plugin::Reload)
    .method("Shutdown", &Plugin::Shutdown);
}

static List<std::shared_ptr<Plugin>> s_plugins;

bool PluginManager::Initialize()
{
    if (!Module::Load("[root]/libs"))
    {
        BX_LOGE("Failed to load plugin libs!");
        return false;
    }

    const auto& derived = rttr::type::get<Plugin>().get_derived_classes();
    for (const auto& type : derived)
    {
        rttr::variant var = type.create();
        auto instance = var.get_value<std::shared_ptr<Plugin>>();

        s_plugins.emplace_back(instance);
    }

    for (const auto& plugin : s_plugins)
    {
        if (!plugin->Initialize())
        {
            BX_LOGE("One or more plugins failed to initialize!");
            return false;
        }
    }

    return true;
}

void PluginManager::Reload()
{
    for (const auto& plugin : s_plugins)
    {
        plugin->Reload();
    }
}

void PluginManager::Shutdown()
{
    for (const auto& plugin : s_plugins)
    {
        plugin->Shutdown();
    }
}