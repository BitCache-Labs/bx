#include <engine/engine.hpp>
#include <engine/log.hpp>
#include <engine/time.hpp>
#include <engine/file.hpp>
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

    if (!Initialize(app))
    {
        BX_LOGE(Engine, "Failed to initialize engine module!");
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
        Debug::Get().ClearDraws();
#endif

#ifdef EDITOR_BUILD
        OnGui(app);
#endif

        Graphics::Get().EndFrame();
        Window::Get().Display();
    }

    Shutdown(app);

    BX_LOGD(Engine, "Engine exit.");
    return EXIT_SUCCESS;
}

bool Engine::Initialize(Application& app) noexcept
{
    app.Configure();

    BX_LOGD(Engine, "Engine initializing ...");

    if (!File::Get().Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize file module!");
        return false;
    }

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

    BX_LOGD(Engine, "Initializing application...");
    if (!app.Initialize())
    {
        BX_LOGE(Engine, "Failed to initialize application!");
        return EXIT_FAILURE;
    }
    BX_LOGD(Engine, "Application initialized successfully.");

    return true;
}

void Engine::Shutdown(Application& app) noexcept
{
    BX_LOGD(Engine, "Application shutting down...");
    app.Shutdown();
    BX_LOGD(Engine, "Application shutdown complete.");

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

    File::Get().Shutdown();

    app.ClearScenes();

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

static const StringView g_coreSrc = R"(
import "meta" for Meta

class Time {
    foreign static time
    foreign static deltaTime
}

foreign class DataTarget {
    construct new(i) {}
    
    foreign static none
    foreign static system
    foreign static debug
    foreign static game
    foreign static player
}

class Data {
    static getBool(name, value) { getBool(name, value, DataTarget.game) }
    static getInt(name, value) { getInt(name, value, DataTarget.game) }
    static getUInt(name, value) { getUInt(name, value, DataTarget.game) }
    static getFloat(name, value) { getFloat(name, value, DataTarget.game) }
    static getDouble(name, value) { getDouble(name, value, DataTarget.game) }
    static getString(name, value) { getString(name, value, DataTarget.game) }

    static setBool(name, value) { setBool(name, value, DataTarget.game) }
    static setInt(name, value) { setInt(name, value, DataTarget.game) }
    static setUInt(name, value) { setUInt(name, value, DataTarget.game) }
    static setFloat(name, value) { setFloat(name, value, DataTarget.game) }
    static setDouble(name, value) { setDouble(name, value, DataTarget.game) }
    static setString(name, value) { setString(name, value, DataTarget.game) }
    
    foreign static getBool(name, value, target)
    foreign static getInt(name, value, target)
    foreign static getUInt(name, value, target)
    foreign static getFloat(name, value, target)
    foreign static getDouble(name, value, target)
    foreign static getString(name, value, target)

    foreign static setBool(name, value, target)
    foreign static setInt(name, value, target)
    foreign static setUInt(name, value, target)
    foreign static setFloat(name, value, target)
    foreign static setDouble(name, value, target)
    foreign static setString(name, value, target)
}

class File {
    foreign static readTextFile(filename)
    foreign static writeTextFile(filename, text)
    foreign static exists(filename)
}

// TODO: Move these

class Device {
    static PlatformPC      { 0 }
    static PlatformPS5     { 1 }
    static PlatformSwitch  { 2 }

    //foreign static getPlatform()
    //foreign static canClose()
    //foreign static requestClose()
}

foreign class Timer {
    construct new() {}
    
    foreign start()
    foreign elapsed()
}

//foreign class Resource {
//    foreign static create(type, filename)
//
//    foreign getData(type)
//    foreign handle
//    foreign isValid
//    foreign isLoaded
//
//    foreign load(filename)
//    //foreign loadData(data)
//    foreign save(filename)
//    foreign unload()    
//}
)";