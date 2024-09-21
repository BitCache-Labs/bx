#pragma once

struct View
{
	using InitializeFn = bool(*)(bool& isShown);
	using ShutdownFn = void(*)(bool& isShown);

	using OnReloadFn = void(*)();
	using OnPlayFn = void(*)();
	using OnStopFn = void(*)();
	using OnPauseFn = void(*)(bool);

	using OnToolbarFn = void(*)(bool& isShown);
	using OnGuiFn = void(*)(bool& isShown);

	InitializeFn Initialize = nullptr;
	ShutdownFn Shutdown = nullptr;

	OnReloadFn OnReload = nullptr;
	OnPlayFn OnPlay = nullptr;
	OnStopFn OnStop = nullptr;
	OnPauseFn OnPause = nullptr;

	OnToolbarFn OnToolbar = nullptr;
	OnGuiFn OnGui = nullptr;

	bool isShown = false;
};