#include <bx/engine/application.hpp>

#include "bx/engine/data.hpp"
#include "bx/engine/resource.hpp"
#include "bx/engine/script.hpp"

#include <bx/engine/plugin.hpp>
#include <bx/core/memory.hpp>
#include <bx/core/log.hpp>
#include <bx/core/time.hpp>
#include <bx/core/profiler.hpp>
#include <bx/core/module.hpp>

#include <bx/core/file.hpp>
#include <bx/platform/input.hpp>
#include <bx/platform/window.hpp>
#include <bx/platform/graphics.hpp>
#include <bx/platform/audio.hpp>
#include <bx/engine/imgui.hpp>

#ifdef BX_EDITOR_BUILD
#include <bx/editor/toolbar.hpp>
#endif

static bool g_running = true;

bool Application::IsRunning()
{
	return g_running && Window::Get().IsOpen();
}

void Application::Close()
{
	g_running = false;
}

void Application::Reload()
{
	PluginManager::Reload();
#ifdef BX_EDITOR_BUILD
	Toolbar::Reload();
#endif
}

int Application::Launch(const AppConfig& config)
{
#ifdef MEMORY_CUSTOM_CONTAINERS
	Memory::Initialize();
#endif

	Time::Initialize();
	File::Initialize(config.argv[1]);
	Data::Initialize();

	ResourceManager::Initialize();
	//Script::Initialize();

	Module::Load("modules");
	
	Window::Get().Initialize();
	Input::Get().Initialize();
	Graphics::Get().Initialize();
	Audio::Get().Initialize();
	
	if (!PluginManager::Initialize())
	{
		BX_LOGE("Failed to initialize plugins!");
		return EXIT_FAILURE;
	}

	ImGuiManager::Initialize();

#ifdef BX_EDITOR_BUILD
	Toolbar::Initialize();
#endif

	while (IsRunning())
	{
		Tick();
	}

#ifdef BX_EDITOR_BUILD
	Toolbar::Shutdown();
#endif

	PluginManager::Shutdown();

	Audio::Get().Shutdown();
	ImGuiManager::Shutdown();
	Graphics::Get().Shutdown();
	Input::Get().Shutdown();
	Window::Get().Shutdown();

	//Script::Shutdown();
	ResourceManager::Shutdown();
	Data::Shutdown();
	//File::Shutdown();
	//Time::Shutdown();

#ifdef MEMORY_CUSTOM_CONTAINERS
	Memory::Shutdown();
#endif

	return EXIT_SUCCESS;
}

void Application::Tick()
{
	Profiler::Update();
	//Script::CollectGarbage();
	Time::Update();
	Window::Get().PollEvents();
	Input::Get().PollEvents();

	Graphics::Get().NewFrame();
	ImGuiManager::NewFrame();

#ifdef BX_EDITOR_BUILD
	Toolbar::Present();
#endif

	ImGuiManager::EndFrame();
	Graphics::Get().EndFrame();

	Window::Get().Display();
}