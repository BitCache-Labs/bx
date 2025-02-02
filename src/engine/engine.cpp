#include <engine/engine.hpp>
#include <engine/log.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <engine/graphics.hpp>
#include <engine/script.hpp>
#include <engine/online.hpp>
#include <engine/debug.hpp>

#ifdef EDITOR_BUILD
#include <editor/editor.hpp>
#endif

BX_MODULE_DEFINE(Engine)

bool Engine::IsRunning() const noexcept
{
    return m_isRunning && Window::Get().IsOpen();
}

void Engine::Close() noexcept
{
    m_isRunning = false;
}

int Engine::Run(int argc, char** args, Application& app)
{
    BX_LOGD(Engine, "Engine starting...");

    app.Configure();
    if (!Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize engine module!");
        return EXIT_FAILURE;
    }

    BX_LOGD(Engine, "Initializing application...");
    if (!app.Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize application!");
        return EXIT_FAILURE;
    }
    BX_LOGD(Engine, "Application initialized successfully.");

    Time::Get().Start();
    while (IsRunning())
    {
        Time::Get().Update();
        Window::Get().PollEvents();
        Online::Get().Update();
        app.Update();

        Graphics::Get().NewFrame();
        app.Render();

#if defined(EDITOR_BUILD) || defined(DEBUG_BUILD)
        Debug::Get().ClearDraws();
#endif

#ifdef EDITOR_BUILD
        OnGui(app);
#endif

        Graphics::Get().EndFrame();
        Window::Get().Display();
    }

    app.Shutdown();
    Shutdown();

    BX_LOGD(Engine, "Engine exit.");
    return EXIT_SUCCESS;
}

bool Engine::Initialize() noexcept
{
    BX_LOGD(Engine, "Engine initializing ...");

    if (!Online::Get().Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize online module!");
        return false;
    }

    if (!Window::Get().Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize window module!");
        return false;
    }

    if (!Graphics::Get().Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize graphics module!");
        return false;
    }

    if (!Script::Get().Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize script module!");
        return false;
    }

#if defined(EDITOR_BUILD) || defined(DEBUG_BUILD)
    if (!Debug::Get().Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize debug module!");
        return false;
    }
#endif

#ifdef EDITOR_BUILD
    if (!Editor::Get().Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize editor module!");
        return false;
    }
#endif

    BX_LOGD(Engine, "Engine initialized successfully.");
    return true;
}

void Engine::Shutdown() noexcept
{
    BX_LOGD(Engine, "Engine shutting down ...");

#ifdef EDITOR_BUILD
    Editor::Get().Shutdown();
#endif

#if defined(EDITOR_BUILD) || defined(DEBUG_BUILD)
    Debug::Get().Shutdown();
#endif

    Script::Get().Shutdown();
    Graphics::Get().Shutdown();
    Window::Get().Shutdown();
    Online::Get().Shutdown();

    BX_LOGD(Engine, "Engine shutdown complete.");
}

#ifdef EDITOR_BUILD
void Engine::OnGui(Application& app) noexcept
{
    auto* editorApp = dynamic_cast<EditorApplication*>(&app);
    if (!editorApp)
        BX_LOGE(Engine, "Invalid application type for editor build!");
    else
        Editor::Get().OnGui(*editorApp);
}
#endif