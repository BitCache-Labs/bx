#include "bx/editor/view.hpp"

#include <bx/containers/list.hpp>

static List<MenuBarCallbackFn> g_menuBarCallbacks;
static List<View*> g_views;

void ViewManager::AddMenuBarCallback(const MenuBarCallbackFn& menuBarCallback)
{
	g_menuBarCallbacks.emplace_back(menuBarCallback);
}

void ViewManager::AddView(View* view)
{
	g_views.emplace_back(view);

	view->Initialize();
}

void ViewManager::Reload()
{
	for (auto view : g_views)
	{
		view->OnReload();
	}
}

void ViewManager::Present()
{
	for (auto menuBarCallback : g_menuBarCallbacks)
	{
		menuBarCallback();
	}

	for (auto view : g_views)
	{
		view->OnPresent();
	}
}