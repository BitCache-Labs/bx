#include <bx/core/application.hpp>

#include "bx/engine/data.hpp"
#include "bx/engine/resource.hpp"
#include "bx/engine/script.hpp"

#include <bx/core/plugin.hpp>
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
#include <bx/editor/assets.hpp>
#include <bx/editor/toolbar.hpp>
#include <bx/editor/views/profiler_view.hpp>
#include <bx/editor/views/data_view.hpp>
#include <bx/editor/views/assets_view.hpp>
#include <bx/editor/views/settings_view.hpp>
#include <bx/editor/views/console_view.hpp>
#endif

void Application::Tick()
{
	//PROFILE_SECTION("Application");

	Profiler::Update();
	Script::CollectGarbage();
	Time::Update();
	Window::PollEvents();
	Input::Poll();

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

	Graphics::NewFrame();
	ImGuiImpl::NewFrame();

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

	ImGuiImpl::EndFrame();
	Graphics::EndFrame();

	Window::Display();

#ifdef BX_EDITOR_BUILD
	AssetManager::Refresh();
#endif
}