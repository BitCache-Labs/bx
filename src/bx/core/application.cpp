#include "bx/core/application.hpp"

#include "bx/core/macros.hpp"
#include "bx/core/plugin.hpp"
#include <bx/core/memory.hpp>
#include <bx/core/log.hpp>
#include <bx/core/time.hpp>
#include <bx/core/profiler.hpp>

#include <bx/platform/file.hpp>
#include <bx/platform/input.hpp>
#include <bx/platform/window.hpp>
#include <bx/platform/graphics.hpp>
#include <bx/platform/audio.hpp>
#include <bx/platform/imgui.hpp>

#ifdef BX_EDITOR_BUILD
#include <bx/editor/view.hpp>
#endif

static bool g_running = true;

bool Application::IsRunning()
{
	return g_running && Window::IsOpen();
}

void Application::Close()
{
	g_running = false;
}

void Application::Reload()
{
	PluginManager::Reload();

#ifdef BX_EDITOR_BUILD
	ViewManager::Reload();
#endif
}

int Application::Launch(const AppConfig& config)
{
#ifdef MEMORY_CUSTOM_CONTAINERS
	Memory::Initialize();
#endif

	Time::Initialize();
	File::Initialize();

	Window::Initialize();
	Input::Initialize();
	Graphics::Initialize();
	ImGuiImpl::Initialize();
	Audio::Initialize();

	if (!Application::Configure(config))
	{
		BX_LOGE("Failed to initialize application!");
		return EXIT_FAILURE;
	}

	if (!PluginManager::Initialize())
	{
		BX_LOGE("Failed to initialize plugins!");
		return EXIT_FAILURE;
	}

	while (IsRunning())
	{
		Tick();
	}

	PluginManager::Shutdown();

	Audio::Shutdown();
	ImGuiImpl::Shutdown();
	Graphics::Shutdown();
	Input::Shutdown();
	Window::Shutdown();

	//File::Shutdown();
	//Time::Shutdown();

#ifdef MEMORY_CUSTOM_CONTAINERS
	Memory::Shutdown();
#endif

	return EXIT_SUCCESS;
}