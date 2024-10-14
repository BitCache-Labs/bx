#pragma once

#include <bx/bx.hpp>

class BX_API Toolbar
{
public:
	static bool Initialize();
	static void Reload();
	static void Shutdown();

	static void Present();
};