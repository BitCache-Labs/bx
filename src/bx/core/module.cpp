#include "bx/engine/core/module.hpp"
#include "bx/engine/containers/list.hpp"

struct ModuleHandle
{
	u32 order = 0;
	ModuleInitializeFn initializeFn = nullptr;
	ModuleReloadFn reloadFn = nullptr;
	ModuleShutdownFn shutdownFn = nullptr;

	bool operator<(const ModuleHandle& other) const { return order < other.order; }
};

static List<ModuleHandle> s_modules;

void Module::Register(u32 order, ModuleInitializeFn initializeFn, ModuleReloadFn reloadFn, ModuleShutdownFn shutdownFn)
{
	ModuleHandle moduleHandle;
	moduleHandle.order = order;
	moduleHandle.initializeFn = initializeFn;
	moduleHandle.reloadFn = reloadFn;
	moduleHandle.shutdownFn = shutdownFn;

	auto it = std::lower_bound(s_modules.begin(), s_modules.end(), moduleHandle);
	s_modules.insert(it, moduleHandle);
}

void Module::Initialize()
{
	for (SizeType i = 0; i < s_modules.size(); ++i)
	{
		const auto& handle = s_modules[i];
		if (handle.initializeFn)
			handle.initializeFn();
	}
}

void Module::Reload()
{
	for (SizeType i = s_modules.size(); i-- > 0;)
	{
		const auto& handle = s_modules[i];
		if (handle.reloadFn)
			handle.reloadFn();
	}
}

void Module::Shutdown()
{
	for (SizeType i = s_modules.size(); i-- > 0;)
	{
		const auto& handle = s_modules[i];
		if (handle.shutdownFn)
			handle.shutdownFn();
	}
}
