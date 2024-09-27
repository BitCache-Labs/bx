#pragma once

#include "bx/editor/selection.hpp"
#include "bx/editor/command.hpp"

using MenuBarCallbackFn = void(*)();

class View
{
public:
	virtual ~View() {}

	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;

	virtual void OnReload() = 0;
	virtual void OnPresent() = 0;
};

class ViewManager
{
public:
	static void AddMenuBarCallback(const MenuBarCallbackFn& callback);
	static void AddView(View* view);

	static void Reload();
	static void Present();
};