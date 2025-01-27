#include <engine/engine.hpp>
#include <engine/log.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <engine/graphics.hpp>
#include <engine/online.hpp>

#ifdef EDITOR_BUILD
#include <editor/editor.hpp>
#endif

Engine& Engine::Get() noexcept
{
    static Engine instance;
    return instance;
}

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
    LOGD(Engine, "Application starting...");

    app.Configure();
    if (!Initialize() || !app.Initialize())
    {
        LOGE(Engine, "Failed to initialize application!");
        return EXIT_FAILURE;
    }

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
        DebugDraw::Get().Clear();
#endif

#ifdef EDITOR_BUILD
        OnGui(app);
#endif

        Graphics::Get().EndFrame();
        Window::Get().Display();
    }

    app.Shutdown();
    Shutdown();

    LOGD(Engine, "Application exit.");
    return EXIT_SUCCESS;
}

bool Engine::Initialize() noexcept
{
    LOGD(Engine, "Initializing application...");

    if (!Online::Get().Initialize())
    {
        LOGE(Engine, "Failed to initialize online!");
        return false;
    }

    if (!Window::Get().Initialize())
    {
        LOGE(Engine, "Failed to initialize window!");
        return false;
    }

    if (!Graphics::Get().Initialize())
    {
        LOGE(Engine, "Failed to initialize graphics!");
        return false;
    }

#if defined(EDITOR_BUILD) || defined(DEBUG_BUILD)
    if (!DebugDraw::Get().Initialize())
    {
        LOGE(Engine, "Failed to initialize debug draw!");
        return false;
    }
#endif

#ifdef EDITOR_BUILD
    if (!Editor::Get().Initialize())
    {
        LOGE(Engine, "Failed to initialize editor!");
        return false;
    }
#endif

    LOGD(Engine, "Application initialized successfully.");
    return true;
}

void Engine::Shutdown() noexcept
{
    LOGD(Engine, "Shutting down application...");

#ifdef EDITOR_BUILD
    Editor::Get().Shutdown();
#endif

#if defined(EDITOR_BUILD) || defined(DEBUG_BUILD)
    DebugDraw::Get().Shutdown();
#endif

    Graphics::Get().Shutdown();
    Window::Get().Shutdown();
    Online::Get().Shutdown();

    LOGD(Engine, "Application shutdown complete.");
}

#ifdef EDITOR_BUILD
void Engine::OnGui(Application& app) noexcept
{
    auto* editorApp = dynamic_cast<EditorApplication*>(&app);
    if (!editorApp)
        LOGE(Engine, "Invalid application type for editor build!");
    else
        Editor::Get().OnGui(*editorApp);
}
#endif