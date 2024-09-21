#include "bx/core/runtime.hpp"

#include "bx/core/macros.hpp"
#include "bx/core/application.hpp"
#include "bx/core/plugin.hpp"

static bool g_running = true;

int main(int argc, char** argv)
{
	return Runtime::Launch(argc, argv);
}

bool Runtime::IsRunning()
{
	return g_running;
}

void Runtime::Close()
{
	g_running = false;
}

void Runtime::Reload()
{
	PluginManager::Reload();
}

int Runtime::Launch(int argc, char** argv)
{
	if (!Application::Initialize())
	{
		BX_LOGE("Failed to initialize application!");
		return EXIT_FAILURE;
	}

	if (!PluginManager::Initialize())
	{
		BX_LOGE("Failed to initialize plugins!");
		return EXIT_FAILURE;
	}

	while (IsRunning())
	{
		PluginManager::Tick();
	}

	PluginManager::Shutdown();
	Application::Shutdown();

	return EXIT_SUCCESS;
}