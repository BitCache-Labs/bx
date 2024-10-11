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
#include <bx/editor/assets.hpp>
#include <bx/editor/toolbar.hpp>
#include <bx/editor/view.hpp>
#include <bx/editor/views/profiler_view.hpp>
//#include <bx/editor/views/data_view.hpp>
//#include <bx/editor/views/assets_view.hpp>
//#include <bx/editor/views/settings_view.hpp>
//#include <bx/editor/views/console_view.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <IconsFontAwesome5.h>

static void InitializeImGui()
{
	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	const float uiScale = 1.0f;
	const float fontSize = 14.0f;
	const float iconSize = 12.0f;

	ImFontConfig config;
	config.OversampleH = 8;
	config.OversampleV = 8;
	io.Fonts->AddFontFromFileTTF(FREE_FONTS_DROID_SANS, fontSize * uiScale, &config);

	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 }; // will not be copied by AddFont* so keep in scope.
	config.MergeMode = true;
	config.OversampleH = 8;
	config.OversampleV = 8;
	io.Fonts->AddFontFromFileTTF(FONT_AWESOME_6_FREE_SOLID_900, iconSize * uiScale, &config, icons_ranges);
}

static void EngineMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Views"))
		{
			if (ImGui::MenuItem("Assets"))
			{
				//ViewManager::AddView(new AssetsView());
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Profiler"))
			{
				ViewManager::AddView(new ProfilerView());
			}

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::ShowDemoWindow();
}
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
	Data::Initialize();

	ResourceManager::Initialize();
	//Script::Initialize();

	Module::Load();
	
	Window::Get().Initialize();
	Input::Get().Initialize();
	Graphics::Get().Initialize();
	ImGuiManager::Initialize();
	Audio::Get().Initialize();

#ifdef BX_EDITOR_BUILD
	InitializeImGui();

	AssetManager::Initialize();

	ViewManager::AddMenuBarCallback(&EngineMenuBar);
	//ViewManager::Initialize();

	String scenePath = Data::GetString("Current Scene", "", DataTarget::EDITOR);
	if (!scenePath.empty())
	{
		scenePath = Data::GetString("Main Scene", "[assets]/main.scene", DataTarget::GAME);
		Data::SetString("Current Scene", scenePath, DataTarget::EDITOR);
	}

	Reload();
	//Scene::Load(scenePath);
#else
	Window::Get().SetCursorMode(CursorMode::DISABLED);

	const String& mainScene = Data::GetString("Main Scene", "[assets]/main.scene", DataTarget::GAME);
	//Scene::Load(mainScene);
#endif

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

#ifdef BX_EDITOR_BUILD
	//ViewManager::Shutdown();
	AssetManager::Shutdown();
#endif

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
	//PROFILE_SECTION("Application");

	Profiler::Update();
	//Script::CollectGarbage();
	Time::Update();
	Window::Get().PollEvents();
	Input::Get().PollEvents();

#ifdef BX_EDITOR_BUILD
//	if (Toolbar::IsPlaying() && (Toolbar::ConsumeNextFrame() || !Toolbar::IsPaused()))
//	{
//		SystemManager::Update();
//		//Scene::GetCurrent().Update();
//		Script::Update();
//	}
#else
//	SystemManager::Update();
//	//Scene::GetCurrent().Update();
//	Script::Update();
#endif

	Graphics::Get().NewFrame();
	ImGuiManager::NewFrame();

#ifdef BX_EDITOR_BUILD
//	if (Toolbar::IsPlaying() && !Toolbar::IsPaused())
//	{
//		SystemManager::Render();
//		Script::Render();
//	}
//	Toolbar::Present();
	ViewManager::Present();
#else
//	SystemManager::Render();
//	Script::Render();
#endif

	ImGuiManager::EndFrame();
	Graphics::Get().EndFrame();

	Window::Get().Display();

#ifdef BX_EDITOR_BUILD
	AssetManager::Refresh();
#endif
}