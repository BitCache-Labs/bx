#pragma once

using ViewInitializeCallback = void(*)();
using ViewShutdownCallback = void(*)();
using ViewPresentCallback = void(*)(bool&);

class View
{
public:
	static void Initialize();
	static void Shutdown();

	static void Present();

	static void Add(ViewInitializeCallback initialize, ViewShutdownCallback shutdown, ViewPresentCallback present);
};