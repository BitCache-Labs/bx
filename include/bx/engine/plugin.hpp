#pragma once

#include <bx/bx.hpp>
#include <rttr/rttr_enable.h>

class BX_API Plugin
{
	RTTR_ENABLE()

public:
	virtual ~Plugin() = default;

	virtual bool Initialize() = 0;
	virtual void Reload() = 0;
	virtual void Shutdown() = 0;
};

class BX_API PluginManager
{
public:
	static bool Initialize();
	static void Reload();
	static void Shutdown();
};