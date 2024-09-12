#pragma once

#include "bx/engine/core/byte_types.hpp"

using ModuleInitializeFn = bool(*)();
using ModuleReloadFn = void(*)();
using ModuleShutdownFn = void(*)();

class Module
{
public:
	template <typename T>
	static void Register(u32 order)
	{
		Register(order, T::Initialize, T::Reload, T::Shutdown);
	}

	static void Register(u32 order, ModuleInitializeFn initialize, ModuleReloadFn reload, ModuleShutdownFn shutdown);

private:
	friend class Runtime;

	static void Initialize();
	static void Reload();
	static void Shutdown();
};