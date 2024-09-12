#include "bx/engine/modules/imgui.hpp"

#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/file.hpp"
#include "bx/engine/core/profiler.hpp"
#include "bx/engine/modules/window.hpp"
#include "bx/engine/modules/graphics.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

//#define IMGUI_CUSTOM_IMPL
// TODO: This is a temporary patch to make imgui work while the custom implementation is being built
#ifndef IMGUI_CUSTOM_IMPL

//#define IMGUI_IMPL_OPENGL_ES3

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#ifdef BX_WINDOW_GLFW_BACKEND
#include "bx/engine/modules/window/backend/window_glfw.hpp"
#endif

bool ImGuiImpl::Initialize()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;        // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

#ifdef BX_WINDOW_GLFW_BACKEND
    if (!ImGui_ImplGlfw_InitForOpenGL(WindowGLFW::GetWindowPtr(), true))
    {
        BX_LOGE("Failed to initialize ImGui GLFW backend!");
        return false;
    }
#endif

#if defined BX_GRAPHICS_OPENGL_BACKEND
    if (!ImGui_ImplOpenGL3_Init("#version 460 core\n"))
#elif defined BX_GRAPHICS_OPENGLES_BACKEND
    if (!ImGui_ImplOpenGL3_Init("#version 300 es\n"))
#endif
    {
        BX_LOGE("Failed to initialize ImGui OpenGL backend!");
        return false;
    }

    //ImGui::CreateContext();
    //ImPlot::CreateContext();
    //ImGui_Impl_Init();
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //
    //float ys;
    //float xs;
    //glfwGetWindowContentScale(device::get_window(), &xs, &ys);
    //ui_scale = (xs + ys) / 2.0f;
    //
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //
    //ImGuiIO& io = ImGui::GetIO();
    //
    //const float UIScale = ui_scale;
    //const float fontSize = 14.0f;
    //const float iconSize = 12.0f;

    //ImFontConfig config;
    //config.OversampleH = 8;
    //config.OversampleV = 8;
    //io.Fonts->AddFontFromFileTTF(fileio::get_path("[games]/shared/fonts/DroidSans.ttf").c_str(), fontSize * UIScale, &config);
    //static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // will not be copied by AddFont* so keep in scope.
    //config.MergeMode = true;
    //config.OversampleH = 8;
    //config.OversampleV = 8;

    //String fontpath = fileio::get_path("[games]/shared/fonts/FontAwesome5FreeSolid900.otf");
    //io.Fonts->AddFontFromFileTTF(fontpath.c_str(), iconSize * UIScale, &config, icons_ranges);

#ifdef BX_EDITOR_BUILD
    const String iniPath = File::GetPath("[editor]/imgui.ini");
    const char* constStr = iniPath.c_str();
    const SizeType pathSize = iniPath.size() + 1;
    char* str = new char[pathSize];

    strncpy(str, constStr, pathSize);
    str[pathSize - 1] = '\0';

    io.IniFilename = str;

#else
    io.IniFilename = NULL;
#endif

    return true;
}

void ImGuiImpl::Reload()
{
}

void ImGuiImpl::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiImpl::NewFrame()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiImpl::EndFrame()
{
    GraphicsHandle renderTarget = Graphics::GetCurrentBackBufferRT();
    GraphicsHandle depthStencil = Graphics::GetDepthBuffer();
    Graphics::SetRenderTarget(renderTarget, depthStencil);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

#endif // !IMGUI_CUSTOM_IMPL


#ifdef IMGUI_CUSTOM_IMPL
// -----------------------------------------------------
// ---------- ImGui Window Implementation --------------
// -----------------------------------------------------
// TODO: Remove glfw dependency, Window.hpp should provide an interface complete enough

#include "Engine/Modules/Window.hpp"

#ifdef WINDOW_GLFW_BACKEND
#include <GLFW/glfw3.h>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>   // for glfwGetWin32Window()
#endif
#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
//#include <GLFW/glfw3native.h>   // for glfwGetCocoaWindow()
#endif

#define GLFW_HAS_WINDOW_TOPMOST       (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ GLFW_FLOATING
#define GLFW_HAS_WINDOW_HOVERED       (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ GLFW_HOVERED
#define GLFW_HAS_WINDOW_ALPHA         (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwSetWindowOpacity
#define GLFW_HAS_PER_MONITOR_DPI      (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwGetMonitorContentScale
#define GLFW_HAS_VULKAN               (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ glfwCreateWindowSurface
#define GLFW_HAS_FOCUS_WINDOW         (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ glfwFocusWindow
#define GLFW_HAS_FOCUS_ON_SHOW        (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ GLFW_FOCUS_ON_SHOW
#define GLFW_HAS_MONITOR_WORK_AREA    (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwGetMonitorWorkarea
#define GLFW_HAS_OSX_WINDOW_POS_FIX   (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 + GLFW_VERSION_REVISION * 10 >= 3310) // 3.3.1+ Fixed: Resizing window repositions it on MacOS #1553
#ifdef GLFW_RESIZE_NESW_CURSOR        // Let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2019-11-29 (cursors defines) // FIXME: Remove when GLFW 3.4 is released?
#define GLFW_HAS_NEW_CURSORS          (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3400) // 3.4+ GLFW_RESIZE_ALL_CURSOR, GLFW_RESIZE_NESW_CURSOR, GLFW_RESIZE_NWSE_CURSOR, GLFW_NOT_ALLOWED_CURSOR
#else
#define GLFW_HAS_NEW_CURSORS          (0)
#endif
#ifdef GLFW_MOUSE_PASSTHROUGH         // Let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2020-07-17 (passthrough)
#define GLFW_HAS_MOUSE_PASSTHROUGH    (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3400) // 3.4+ GLFW_MOUSE_PASSTHROUGH
#else
#define GLFW_HAS_MOUSE_PASSTHROUGH    (0)
#endif
#define GLFW_HAS_GAMEPAD_API          (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwGetGamepadState() new api
#define GLFW_HAS_GET_KEY_NAME         (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ glfwGetKeyName()

#endif

//#define IMGUI_UNLIMITED_FRAME_RATE

struct ImGuiImpl_WindowData
{
    GLFWwindow* Window;
    //GlfwClientApi           ClientApi;
    double                  Time;
    GLFWwindow* MouseWindow;
    GLFWcursor* MouseCursors[ImGuiMouseCursor_COUNT];
    ImVec2                  LastValidMousePos;
    GLFWwindow* KeyOwnerWindows[GLFW_KEY_LAST];
    bool                    InstalledCallbacks;
    bool                    WantUpdateMonitors;
#ifdef _WIN32
//    WNDPROC                 GlfwWndProc;
#endif

    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    GLFWwindowfocusfun      PrevUserCallbackWindowFocus;
    GLFWcursorposfun        PrevUserCallbackCursorPos;
    GLFWcursorenterfun      PrevUserCallbackCursorEnter;
    GLFWmousebuttonfun      PrevUserCallbackMousebutton;
    GLFWscrollfun           PrevUserCallbackScroll;
    GLFWkeyfun              PrevUserCallbackKey;
    GLFWcharfun             PrevUserCallbackChar;
    GLFWmonitorfun          PrevUserCallbackMonitor;

    ImGuiImpl_WindowData() { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// - Because glfwPollEvents() process all windows and some events may be called outside of it, you will need to register your own callbacks
//   (passing install_callbacks=false in ImGui_ImplGlfw_InitXXX functions), set the current dear imgui context and then call our callbacks.
// - Otherwise we may need to store a GLFWWindow* -> ImGuiContext* map and handle this in the backend, adding a little bit of extra complexity to it.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static ImGuiImpl_WindowData* ImGuiImpl_WindowGetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGuiImpl_WindowData*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

static const char* ImGuiImpl_GetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGuiImpl_SetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGuiImpl_ViewportData
{
    GLFWwindow* Window;
    bool        WindowOwned;
    int         IgnoreWindowPosEventFrame;
    int         IgnoreWindowSizeEventFrame;

    ImGuiImpl_ViewportData() { Window = nullptr; WindowOwned = false; IgnoreWindowSizeEventFrame = IgnoreWindowPosEventFrame = -1; }
    ~ImGuiImpl_ViewportData() { IM_ASSERT(Window == nullptr); }
};

static void ImGuiImpl_WindowCloseCallback(GLFWwindow* window)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
        viewport->PlatformRequestClose = true;
}

// GLFW may dispatch window pos/size events after calling glfwSetWindowPos()/glfwSetWindowSize().
// However: depending on the platform the callback may be invoked at different time:
// - on Windows it appears to be called within the glfwSetWindowPos()/glfwSetWindowSize() call
// - on Linux it is queued and invoked during glfwPollEvents()
// Because the event doesn't always fire on glfwSetWindowXXX() we use a frame counter tag to only
// ignore recent glfwSetWindowXXX() calls.
static void ImGuiImpl_WindowPosCallback(GLFWwindow* window, int, int)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
    {
        if (ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData)
        {
            bool ignore_event = (ImGui::GetFrameCount() <= vd->IgnoreWindowPosEventFrame + 1);
            //data->IgnoreWindowPosEventFrame = -1;
            if (ignore_event)
                return;
        }
        viewport->PlatformRequestMove = true;
    }
}

static void ImGuiImpl_WindowSizeCallback(GLFWwindow* window, int, int)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
    {
        if (ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData)
        {
            bool ignore_event = (ImGui::GetFrameCount() <= vd->IgnoreWindowSizeEventFrame + 1);
            //data->IgnoreWindowSizeEventFrame = -1;
            if (ignore_event)
                return;
        }
        viewport->PlatformRequestResize = true;
    }
}

static ImGuiKey ImGuiImpl_KeyToImGuiKey(int key)
{
    switch (key)
    {
    case GLFW_KEY_TAB: return ImGuiKey_Tab;
    case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
    case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
    case GLFW_KEY_UP: return ImGuiKey_UpArrow;
    case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
    case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
    case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
    case GLFW_KEY_HOME: return ImGuiKey_Home;
    case GLFW_KEY_END: return ImGuiKey_End;
    case GLFW_KEY_INSERT: return ImGuiKey_Insert;
    case GLFW_KEY_DELETE: return ImGuiKey_Delete;
    case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
    case GLFW_KEY_SPACE: return ImGuiKey_Space;
    case GLFW_KEY_ENTER: return ImGuiKey_Enter;
    case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
    case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
    case GLFW_KEY_COMMA: return ImGuiKey_Comma;
    case GLFW_KEY_MINUS: return ImGuiKey_Minus;
    case GLFW_KEY_PERIOD: return ImGuiKey_Period;
    case GLFW_KEY_SLASH: return ImGuiKey_Slash;
    case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
    case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
    case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
    case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
    case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
    case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
    case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
    case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
    case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
    case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
    case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
    case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
    case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
    case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
    case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
    case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
    case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
    case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
    case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
    case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
    case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
    case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
    case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
    case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
    case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
    case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
    case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
    case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
    case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
    case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
    case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
    case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
    case GLFW_KEY_MENU: return ImGuiKey_Menu;
    case GLFW_KEY_0: return ImGuiKey_0;
    case GLFW_KEY_1: return ImGuiKey_1;
    case GLFW_KEY_2: return ImGuiKey_2;
    case GLFW_KEY_3: return ImGuiKey_3;
    case GLFW_KEY_4: return ImGuiKey_4;
    case GLFW_KEY_5: return ImGuiKey_5;
    case GLFW_KEY_6: return ImGuiKey_6;
    case GLFW_KEY_7: return ImGuiKey_7;
    case GLFW_KEY_8: return ImGuiKey_8;
    case GLFW_KEY_9: return ImGuiKey_9;
    case GLFW_KEY_A: return ImGuiKey_A;
    case GLFW_KEY_B: return ImGuiKey_B;
    case GLFW_KEY_C: return ImGuiKey_C;
    case GLFW_KEY_D: return ImGuiKey_D;
    case GLFW_KEY_E: return ImGuiKey_E;
    case GLFW_KEY_F: return ImGuiKey_F;
    case GLFW_KEY_G: return ImGuiKey_G;
    case GLFW_KEY_H: return ImGuiKey_H;
    case GLFW_KEY_I: return ImGuiKey_I;
    case GLFW_KEY_J: return ImGuiKey_J;
    case GLFW_KEY_K: return ImGuiKey_K;
    case GLFW_KEY_L: return ImGuiKey_L;
    case GLFW_KEY_M: return ImGuiKey_M;
    case GLFW_KEY_N: return ImGuiKey_N;
    case GLFW_KEY_O: return ImGuiKey_O;
    case GLFW_KEY_P: return ImGuiKey_P;
    case GLFW_KEY_Q: return ImGuiKey_Q;
    case GLFW_KEY_R: return ImGuiKey_R;
    case GLFW_KEY_S: return ImGuiKey_S;
    case GLFW_KEY_T: return ImGuiKey_T;
    case GLFW_KEY_U: return ImGuiKey_U;
    case GLFW_KEY_V: return ImGuiKey_V;
    case GLFW_KEY_W: return ImGuiKey_W;
    case GLFW_KEY_X: return ImGuiKey_X;
    case GLFW_KEY_Y: return ImGuiKey_Y;
    case GLFW_KEY_Z: return ImGuiKey_Z;
    case GLFW_KEY_F1: return ImGuiKey_F1;
    case GLFW_KEY_F2: return ImGuiKey_F2;
    case GLFW_KEY_F3: return ImGuiKey_F3;
    case GLFW_KEY_F4: return ImGuiKey_F4;
    case GLFW_KEY_F5: return ImGuiKey_F5;
    case GLFW_KEY_F6: return ImGuiKey_F6;
    case GLFW_KEY_F7: return ImGuiKey_F7;
    case GLFW_KEY_F8: return ImGuiKey_F8;
    case GLFW_KEY_F9: return ImGuiKey_F9;
    case GLFW_KEY_F10: return ImGuiKey_F10;
    case GLFW_KEY_F11: return ImGuiKey_F11;
    case GLFW_KEY_F12: return ImGuiKey_F12;
    default: return ImGuiKey_None;
    }
}

static int ImGuiImpl_KeyToModifier(int key)
{
    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
        return GLFW_MOD_CONTROL;
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
        return GLFW_MOD_SHIFT;
    if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
        return GLFW_MOD_ALT;
    if (key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER)
        return GLFW_MOD_SUPER;
    return 0;
}

static void ImGuiImpl_UpdateKeyModifiers(int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);
}

static void ImGuiImpl_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (bd->PrevUserCallbackMousebutton != nullptr && window == bd->Window)
        bd->PrevUserCallbackMousebutton(window, button, action, mods);

    ImGuiImpl_UpdateKeyModifiers(mods);

    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
}

static void ImGuiImpl_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (bd->PrevUserCallbackScroll != nullptr && window == bd->Window)
        bd->PrevUserCallbackScroll(window, xoffset, yoffset);

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent((float)xoffset, (float)yoffset);
}

static int ImGuiImpl_TranslateUntranslatedKey(int key, int scancode)
{
#if GLFW_HAS_GET_KEY_NAME && !defined(__EMSCRIPTEN__)
    // GLFW 3.1+ attempts to "untranslate" keys, which goes the opposite of what every other framework does, making using lettered shortcuts difficult.
    // (It had reasons to do so: namely GLFW is/was more likely to be used for WASD-type game controls rather than lettered shortcuts, but IHMO the 3.1 change could have been done differently)
    // See https://github.com/glfw/glfw/issues/1502 for details.
    // Adding a workaround to undo this (so our keys are translated->untranslated->translated, likely a lossy process).
    // This won't cover edge cases but this is at least going to cover common cases.
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
        return key;
    const char* key_name = glfwGetKeyName(key, scancode);
    if (key_name && key_name[0] != 0 && key_name[1] == 0)
    {
        const char char_names[] = "`-=[]\\,;\'./";
        const int char_keys[] = { GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_COMMA, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_PERIOD, GLFW_KEY_SLASH, 0 };
        IM_ASSERT(IM_ARRAYSIZE(char_names) == IM_ARRAYSIZE(char_keys));
        if (key_name[0] >= '0' && key_name[0] <= '9') { key = GLFW_KEY_0 + (key_name[0] - '0'); }
        else if (key_name[0] >= 'A' && key_name[0] <= 'Z') { key = GLFW_KEY_A + (key_name[0] - 'A'); }
        else if (key_name[0] >= 'a' && key_name[0] <= 'z') { key = GLFW_KEY_A + (key_name[0] - 'a'); }
        else if (const char* p = strchr(char_names, key_name[0])) { key = char_keys[p - char_names]; }
    }
    // if (action == GLFW_PRESS) printf("key %d scancode %d name '%s'\n", key, scancode, key_name);
#else
    IM_UNUSED(scancode);
#endif
    return key;
}

static void ImGuiImpl_KeyCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (bd->PrevUserCallbackKey != nullptr && window == bd->Window)
        bd->PrevUserCallbackKey(window, keycode, scancode, action, mods);

    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;

    // Workaround: X11 does not include current pressed/released modifier key in 'mods' flags. https://github.com/glfw/glfw/issues/1630
    if (int keycode_to_mod = ImGuiImpl_KeyToModifier(keycode))
        mods = (action == GLFW_PRESS) ? (mods | keycode_to_mod) : (mods & ~keycode_to_mod);
    ImGuiImpl_UpdateKeyModifiers(mods);

    if (keycode >= 0 && keycode < IM_ARRAYSIZE(bd->KeyOwnerWindows))
        bd->KeyOwnerWindows[keycode] = (action == GLFW_PRESS) ? window : nullptr;

    keycode = ImGuiImpl_TranslateUntranslatedKey(keycode, scancode);

    ImGuiIO& io = ImGui::GetIO();
    ImGuiKey imgui_key = ImGuiImpl_KeyToImGuiKey(keycode);
    io.AddKeyEvent(imgui_key, (action == GLFW_PRESS));
    io.SetKeyEventNativeData(imgui_key, keycode, scancode); // To support legacy indexing (<1.87 user code)
}

static void ImGuiImpl_WindowFocusCallback(GLFWwindow* window, int focused)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (bd->PrevUserCallbackWindowFocus != nullptr && window == bd->Window)
        bd->PrevUserCallbackWindowFocus(window, focused);

    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(focused != 0);
}

static void ImGuiImpl_CursorPosCallback(GLFWwindow* window, double x, double y)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (bd->PrevUserCallbackCursorPos != nullptr && window == bd->Window)
        bd->PrevUserCallbackCursorPos(window, x, y);
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        int window_x, window_y;
        glfwGetWindowPos(window, &window_x, &window_y);
        x += window_x;
        y += window_y;
    }
    io.AddMousePosEvent((float)x, (float)y);
    bd->LastValidMousePos = ImVec2((float)x, (float)y);
}

// Workaround: X11 seems to send spurious Leave/Enter events which would make us lose our position,
// so we back it up and restore on Leave/Enter (see https://github.com/ocornut/imgui/issues/4984)
static void ImGuiImpl_CursorEnterCallback(GLFWwindow* window, int entered)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (bd->PrevUserCallbackCursorEnter != nullptr && window == bd->Window)
        bd->PrevUserCallbackCursorEnter(window, entered);
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (entered)
    {
        bd->MouseWindow = window;
        io.AddMousePosEvent(bd->LastValidMousePos.x, bd->LastValidMousePos.y);
    }
    else if (!entered && bd->MouseWindow == window)
    {
        bd->LastValidMousePos = io.MousePos;
        bd->MouseWindow = nullptr;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}

static void ImGuiImpl_CharCallback(GLFWwindow* window, unsigned int c)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (bd->PrevUserCallbackChar != nullptr && window == bd->Window)
        bd->PrevUserCallbackChar(window, c);

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

static void ImGuiImpl_MonitorCallback(GLFWmonitor*, int)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    bd->WantUpdateMonitors = true;
}

static void ImGuiImpl_InstallCallbacks(GLFWwindow* window)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    IM_ASSERT(bd->InstalledCallbacks == false && "Callbacks already installed!");
    IM_ASSERT(bd->Window == window);

    bd->PrevUserCallbackWindowFocus = glfwSetWindowFocusCallback(window, ImGuiImpl_WindowFocusCallback);
    bd->PrevUserCallbackCursorEnter = glfwSetCursorEnterCallback(window, ImGuiImpl_CursorEnterCallback);
    bd->PrevUserCallbackCursorPos = glfwSetCursorPosCallback(window, ImGuiImpl_CursorPosCallback);
    bd->PrevUserCallbackMousebutton = glfwSetMouseButtonCallback(window, ImGuiImpl_MouseButtonCallback);
    bd->PrevUserCallbackScroll = glfwSetScrollCallback(window, ImGuiImpl_ScrollCallback);
    bd->PrevUserCallbackKey = glfwSetKeyCallback(window, ImGuiImpl_KeyCallback);
    bd->PrevUserCallbackChar = glfwSetCharCallback(window, ImGuiImpl_CharCallback);
    bd->PrevUserCallbackMonitor = glfwSetMonitorCallback(ImGuiImpl_MonitorCallback);
    bd->InstalledCallbacks = true;
}

static void ImGuiImpl_UpdateMonitors()
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    int monitors_count = 0;
    GLFWmonitor** glfw_monitors = glfwGetMonitors(&monitors_count);
    platform_io.Monitors.resize(0);
    for (int n = 0; n < monitors_count; n++)
    {
        ImGuiPlatformMonitor monitor;
        int x, y;
        glfwGetMonitorPos(glfw_monitors[n], &x, &y);
        const GLFWvidmode* vid_mode = glfwGetVideoMode(glfw_monitors[n]);
        monitor.MainPos = monitor.WorkPos = ImVec2((float)x, (float)y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float)vid_mode->width, (float)vid_mode->height);
#if GLFW_HAS_MONITOR_WORK_AREA
        int w, h;
        glfwGetMonitorWorkarea(glfw_monitors[n], &x, &y, &w, &h);
        if (w > 0 && h > 0) // Workaround a small GLFW issue reporting zero on monitor changes: https://github.com/glfw/glfw/pull/1761
        {
            monitor.WorkPos = ImVec2((float)x, (float)y);
            monitor.WorkSize = ImVec2((float)w, (float)h);
        }
#endif
#if GLFW_HAS_PER_MONITOR_DPI
        // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
        float x_scale, y_scale;
        glfwGetMonitorContentScale(glfw_monitors[n], &x_scale, &y_scale);
        monitor.DpiScale = x_scale;
#endif
        platform_io.Monitors.push_back(monitor);
    }
    bd->WantUpdateMonitors = false;
}

static void ImGuiImpl_CreateWindow(ImGuiViewport* viewport)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    ImGuiImpl_ViewportData* vd = IM_NEW(ImGuiImpl_ViewportData)();
    viewport->PlatformUserData = vd;

    // GLFW 3.2 unfortunately always set focus on glfwCreateWindow() if GLFW_VISIBLE is set, regardless of GLFW_FOCUSED
    // With GLFW 3.3, the hint GLFW_FOCUS_ON_SHOW fixes this problem
    glfwWindowHint(GLFW_VISIBLE, false);
    glfwWindowHint(GLFW_FOCUSED, false);
#if GLFW_HAS_FOCUS_ON_SHOW
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);
#endif
    glfwWindowHint(GLFW_DECORATED, (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? false : true);
#if GLFW_HAS_WINDOW_TOPMOST
    glfwWindowHint(GLFW_FLOATING, (viewport->Flags & ImGuiViewportFlags_TopMost) ? true : false);
#endif
    GLFWwindow* share_window = nullptr;// (bd->ClientApi == GlfwClientApi_OpenGL) ? bd->Window : nullptr;
    vd->Window = glfwCreateWindow((int)viewport->Size.x, (int)viewport->Size.y, "No Title Yet", nullptr, share_window);
    vd->WindowOwned = true;
    viewport->PlatformHandle = (void*)vd->Window;
#ifdef _WIN32
    //viewport->PlatformHandleRaw = glfwGetWin32Window(vd->Window);
#elif defined(__APPLE__)
    viewport->PlatformHandleRaw = (void*)glfwGetCocoaWindow(vd->Window);
#endif
    glfwSetWindowPos(vd->Window, (int)viewport->Pos.x, (int)viewport->Pos.y);

    // Install GLFW callbacks for secondary viewports
    glfwSetWindowFocusCallback(vd->Window, ImGuiImpl_WindowFocusCallback);
    glfwSetCursorEnterCallback(vd->Window, ImGuiImpl_CursorEnterCallback);
    glfwSetCursorPosCallback(vd->Window, ImGuiImpl_CursorPosCallback);
    glfwSetMouseButtonCallback(vd->Window, ImGuiImpl_MouseButtonCallback);
    glfwSetScrollCallback(vd->Window, ImGuiImpl_ScrollCallback);
    glfwSetKeyCallback(vd->Window, ImGuiImpl_KeyCallback);
    glfwSetCharCallback(vd->Window, ImGuiImpl_CharCallback);
    glfwSetWindowCloseCallback(vd->Window, ImGuiImpl_WindowCloseCallback);
    glfwSetWindowPosCallback(vd->Window, ImGuiImpl_WindowPosCallback);
    glfwSetWindowSizeCallback(vd->Window, ImGuiImpl_WindowSizeCallback);
    //if (bd->ClientApi == GlfwClientApi_OpenGL)
    //{
    //    glfwMakeContextCurrent(vd->Window);
    //    glfwSwapInterval(0);
    //}
}

static void ImGuiImpl_DestroyWindow(ImGuiViewport* viewport)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData)
    {
        if (vd->WindowOwned)
        {
#if !GLFW_HAS_MOUSE_PASSTHROUGH && GLFW_HAS_WINDOW_HOVERED && defined(_WIN32)
            HWND hwnd = (HWND)viewport->PlatformHandleRaw;
            ::RemovePropA(hwnd, "IMGUI_VIEWPORT");
#endif

            // Release any keys that were pressed in the window being destroyed and are still held down,
            // because we will not receive any release events after window is destroyed.
            for (int i = 0; i < IM_ARRAYSIZE(bd->KeyOwnerWindows); i++)
                if (bd->KeyOwnerWindows[i] == vd->Window)
                    ImGuiImpl_KeyCallback(vd->Window, i, 0, GLFW_RELEASE, 0); // Later params are only used for main viewport, on which this function is never called.

            glfwDestroyWindow(vd->Window);
        }
        vd->Window = nullptr;
        IM_DELETE(vd);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = nullptr;
}

// We have submitted https://github.com/glfw/glfw/pull/1568 to allow GLFW to support "transparent inputs".
// In the meanwhile we implement custom per-platform workarounds here (FIXME-VIEWPORT: Implement same work-around for Linux/OSX!)
#if !GLFW_HAS_MOUSE_PASSTHROUGH && GLFW_HAS_WINDOW_HOVERED && defined(_WIN32)
static LRESULT CALLBACK WndProcNoInputs(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if (msg == WM_NCHITTEST)
    {
        // Let mouse pass-through the window. This will allow the backend to call io.AddMouseViewportEvent() properly (which is OPTIONAL).
        // The ImGuiViewportFlags_NoInputs flag is set while dragging a viewport, as want to detect the window behind the one we are dragging.
        // If you cannot easily access those viewport flags from your windowing/event code: you may manually synchronize its state e.g. in
        // your main loop after calling UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your windowing system.
        ImGuiViewport* viewport = (ImGuiViewport*)::GetPropA(hWnd, "IMGUI_VIEWPORT");
        if (viewport->Flags & ImGuiViewportFlags_NoInputs)
            return HTTRANSPARENT;
    }
    return ::CallWindowProc(bd->GlfwWndProc, hWnd, msg, wParam, lParam);
}
#endif

static void ImGuiImpl_ShowWindow(ImGuiViewport* viewport)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;

#if defined(_WIN32)
    // GLFW hack: Hide icon from task bar
    //HWND hwnd = (HWND)viewport->PlatformHandleRaw;
    //if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
    //{
    //    LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
    //    ex_style &= ~WS_EX_APPWINDOW;
    //    ex_style |= WS_EX_TOOLWINDOW;
    //    ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    //}

    // GLFW hack: install hook for WM_NCHITTEST message handler
#if !GLFW_HAS_MOUSE_PASSTHROUGH && GLFW_HAS_WINDOW_HOVERED && defined(_WIN32)
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    ::SetPropA(hwnd, "IMGUI_VIEWPORT", viewport);
    if (bd->GlfwWndProc == nullptr)
        bd->GlfwWndProc = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcNoInputs);
#endif

#if !GLFW_HAS_FOCUS_ON_SHOW
    // GLFW hack: GLFW 3.2 has a bug where glfwShowWindow() also activates/focus the window.
    // The fix was pushed to GLFW repository on 2018/01/09 and should be included in GLFW 3.3 via a GLFW_FOCUS_ON_SHOW window attribute.
    // See https://github.com/glfw/glfw/issues/1189
    // FIXME-VIEWPORT: Implement same work-around for Linux/OSX in the meanwhile.
    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
    {
        ::ShowWindow(hwnd, SW_SHOWNA);
        return;
    }
#endif
#endif

    glfwShowWindow(vd->Window);
}

static ImVec2 ImGuiImpl_GetWindowPos(ImGuiViewport* viewport)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    int x = 0, y = 0;
    glfwGetWindowPos(vd->Window, &x, &y);
    return ImVec2((float)x, (float)y);
}

static void ImGuiImpl_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    vd->IgnoreWindowPosEventFrame = ImGui::GetFrameCount();
    glfwSetWindowPos(vd->Window, (int)pos.x, (int)pos.y);
}

static ImVec2 ImGuiImpl_GetWindowSize(ImGuiViewport* viewport)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    int w = 0, h = 0;
    glfwGetWindowSize(vd->Window, &w, &h);
    return ImVec2((float)w, (float)h);
}

static void ImGuiImpl_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
#if __APPLE__ && !GLFW_HAS_OSX_WINDOW_POS_FIX
    // Native OS windows are positioned from the bottom-left corner on macOS, whereas on other platforms they are
    // positioned from the upper-left corner. GLFW makes an effort to convert macOS style coordinates, however it
    // doesn't handle it when changing size. We are manually moving the window in order for changes of size to be based
    // on the upper-left corner.
    int x, y, width, height;
    glfwGetWindowPos(vd->Window, &x, &y);
    glfwGetWindowSize(vd->Window, &width, &height);
    glfwSetWindowPos(vd->Window, x, y - height + size.y);
#endif
    vd->IgnoreWindowSizeEventFrame = ImGui::GetFrameCount();
    glfwSetWindowSize(vd->Window, (int)size.x, (int)size.y);
}

static void ImGuiImpl_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    glfwSetWindowTitle(vd->Window, title);
}

static void ImGuiImpl_SetWindowFocus(ImGuiViewport* viewport)
{
#if GLFW_HAS_FOCUS_WINDOW
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    glfwFocusWindow(vd->Window);
#else
    // FIXME: What are the effect of not having this function? At the moment imgui doesn't actually call SetWindowFocus - we set that up ahead, will answer that question later.
    (void)viewport;
#endif
}

static bool ImGuiImpl_GetWindowFocus(ImGuiViewport* viewport)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    return glfwGetWindowAttrib(vd->Window, GLFW_FOCUSED) != 0;
}

static bool ImGuiImpl_GetWindowMinimized(ImGuiViewport* viewport)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    return glfwGetWindowAttrib(vd->Window, GLFW_ICONIFIED) != 0;
}

#if GLFW_HAS_WINDOW_ALPHA
static void ImGuiImpl_SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    glfwSetWindowOpacity(vd->Window, alpha);
}
#endif

static void ImGuiImpl_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    //if (bd->ClientApi == GlfwClientApi_OpenGL)
    //    glfwMakeContextCurrent(vd->Window);
}

static void ImGuiImpl_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    //if (bd->ClientApi == GlfwClientApi_OpenGL)
    //{
    //    glfwMakeContextCurrent(vd->Window);
    //    glfwSwapBuffers(vd->Window);
    //}
}

//--------------------------------------------------------------------------------------------------------
// Vulkan support (the Vulkan renderer needs to call a platform-side support function to create the surface)
//--------------------------------------------------------------------------------------------------------

// Avoid including <vulkan.h> so we can build without it
#if GLFW_HAS_VULKAN
#ifndef VULKAN_H_
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)
struct VkAllocationCallbacks;
enum VkResult { VK_RESULT_MAX_ENUM = 0x7FFFFFFF };
#endif // VULKAN_H_
extern "C" { extern GLFWAPI VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface); }
static int ImGuiImpl_CreateVkSurface(ImGuiViewport* viewport, ImU64 vk_instance, const void* vk_allocator, ImU64* out_vk_surface)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    ImGuiImpl_ViewportData* vd = (ImGuiImpl_ViewportData*)viewport->PlatformUserData;
    IM_UNUSED(bd);
    //IM_ASSERT(bd->ClientApi == GlfwClientApi_Vulkan);
    VkResult err = glfwCreateWindowSurface((VkInstance)vk_instance, vd->Window, (const VkAllocationCallbacks*)vk_allocator, (VkSurfaceKHR*)out_vk_surface);
    return (int)err;
}
#endif // GLFW_HAS_VULKAN

static void ImGuiImpl_InitializePlatformInterface()
{
    // Register platform interface (will be coupled with a renderer interface)
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    platformIo.Platform_CreateWindow = ImGuiImpl_CreateWindow;
    platformIo.Platform_DestroyWindow = ImGuiImpl_DestroyWindow;
    platformIo.Platform_ShowWindow = ImGuiImpl_ShowWindow;
    platformIo.Platform_SetWindowPos = ImGuiImpl_SetWindowPos;
    platformIo.Platform_GetWindowPos = ImGuiImpl_GetWindowPos;
    platformIo.Platform_SetWindowSize = ImGuiImpl_SetWindowSize;
    platformIo.Platform_GetWindowSize = ImGuiImpl_GetWindowSize;
    platformIo.Platform_SetWindowFocus = ImGuiImpl_SetWindowFocus;
    platformIo.Platform_GetWindowFocus = ImGuiImpl_GetWindowFocus;
    platformIo.Platform_GetWindowMinimized = ImGuiImpl_GetWindowMinimized;
    platformIo.Platform_SetWindowTitle = ImGuiImpl_SetWindowTitle;
    platformIo.Platform_RenderWindow = ImGuiImpl_RenderWindow;
    platformIo.Platform_SwapBuffers = ImGuiImpl_SwapBuffers;
#if GLFW_HAS_WINDOW_ALPHA
    platformIo.Platform_SetWindowAlpha = ImGuiImpl_SetWindowAlpha;
#endif
#if GLFW_HAS_VULKAN
    platformIo.Platform_CreateVkSurface = ImGuiImpl_CreateVkSurface;
#endif

    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiImpl_ViewportData* vd = IM_NEW(ImGuiImpl_ViewportData)();
    vd->Window = bd->Window;
    vd->WindowOwned = false;
    main_viewport->PlatformUserData = vd;
    main_viewport->PlatformHandle = (void*)bd->Window;
}


static bool ImGuiImpl_InitializeWindow(void* pWindow)
{
#ifdef WINDOW_GLFW_BACKEND
    GLFWwindow* window = (GLFWwindow*)pWindow;
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    
    // Setup backend capabilities flags
    ImGuiImpl_WindowData* bd = IM_NEW(ImGuiImpl_WindowData)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_custom";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
#if GLFW_HAS_MOUSE_PASSTHROUGH || (GLFW_HAS_WINDOW_HOVERED && defined(_WIN32))
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data (optional)
#endif
    
    bd->Window = window;
    bd->Time = 0.0;
    bd->WantUpdateMonitors = true;

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
    
    io.SetClipboardTextFn = ImGuiImpl_SetClipboardText;
    io.GetClipboardTextFn = ImGuiImpl_GetClipboardText;
    io.ClipboardUserData = bd->Window;
#if defined(_WIN32)
    //io.ImeWindowHandle = (void*)glfwGetWin32Window(window);
#endif
    
    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return nullptr and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prevErrorCallback = glfwSetErrorCallback(nullptr);
    bd->MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prevErrorCallback);
    
    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    ImGuiImpl_InstallCallbacks(window);
    
    // Update monitors the first time (note: monitor callback are broken in GLFW 3.2 and earlier, see github.com/glfw/glfw/issues/784)
    ImGuiImpl_UpdateMonitors();
    glfwSetMonitorCallback(ImGuiImpl_MonitorCallback);
    
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    mainViewport->PlatformHandle = (void*)bd->Window;
#ifdef _WIN32
    //mainViewport->PlatformHandleRaw = glfwGetWin32Window(bd->Window);
#elif defined(__APPLE__)
    mainViewport->PlatformHandleRaw = (void*)glfwGetCocoaWindow(bd->Window);
#endif

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGuiImpl_InitializePlatformInterface();
    
    //bd->ClientApi = client_api;
    return true;
#endif
}

static void ImGuiImpl_ShutdownPlatformInterface()
{
    ImGui::DestroyPlatformWindows();
}

static void ImGuiImpl_RestoreCallbacks(GLFWwindow* window)
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    IM_ASSERT(bd->InstalledCallbacks == true && "Callbacks not installed!");
    IM_ASSERT(bd->Window == window);

    glfwSetWindowFocusCallback(window, bd->PrevUserCallbackWindowFocus);
    glfwSetCursorEnterCallback(window, bd->PrevUserCallbackCursorEnter);
    glfwSetCursorPosCallback(window, bd->PrevUserCallbackCursorPos);
    glfwSetMouseButtonCallback(window, bd->PrevUserCallbackMousebutton);
    glfwSetScrollCallback(window, bd->PrevUserCallbackScroll);
    glfwSetKeyCallback(window, bd->PrevUserCallbackKey);
    glfwSetCharCallback(window, bd->PrevUserCallbackChar);
    glfwSetMonitorCallback(bd->PrevUserCallbackMonitor);
    bd->InstalledCallbacks = false;
    bd->PrevUserCallbackWindowFocus = nullptr;
    bd->PrevUserCallbackCursorEnter = nullptr;
    bd->PrevUserCallbackCursorPos = nullptr;
    bd->PrevUserCallbackMousebutton = nullptr;
    bd->PrevUserCallbackScroll = nullptr;
    bd->PrevUserCallbackKey = nullptr;
    bd->PrevUserCallbackChar = nullptr;
    bd->PrevUserCallbackMonitor = nullptr;
}

static void ImGuiImpl_WindowShutdown()
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");

    ImGuiImpl_ShutdownPlatformInterface();

    if (bd->InstalledCallbacks)
        ImGuiImpl_RestoreCallbacks(bd->Window);

    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        glfwDestroyCursor(bd->MouseCursors[cursor_n]);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    IM_DELETE(bd);
}

static void ImGuiImpl_UpdateMouseData()
{
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    if (glfwGetInputMode(bd->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        return;
    }

    ImGuiID mouse_viewport_id = 0;
    const ImVec2 mouse_pos_prev = io.MousePos;
    for (int n = 0; n < platform_io.Viewports.Size; n++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[n];
        GLFWwindow* window = (GLFWwindow*)viewport->PlatformHandle;

#ifdef __EMSCRIPTEN__
        const bool is_window_focused = true;
#else
        const bool is_window_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
#endif
        if (is_window_focused)
        {
            // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
            // When multi-viewports are enabled, all Dear ImGui positions are same as OS positions.
            if (io.WantSetMousePos)
                glfwSetCursorPos(window, (double)(mouse_pos_prev.x - viewport->Pos.x), (double)(mouse_pos_prev.y - viewport->Pos.y));

            // (Optional) Fallback to provide mouse position when focused (ImGui_ImplGlfw_CursorPosCallback already provides this when hovered or captured)
            if (bd->MouseWindow == nullptr)
            {
                double mouse_x, mouse_y;
                glfwGetCursorPos(window, &mouse_x, &mouse_y);
                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
                    // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
                    int window_x, window_y;
                    glfwGetWindowPos(window, &window_x, &window_y);
                    mouse_x += window_x;
                    mouse_y += window_y;
                }
                bd->LastValidMousePos = ImVec2((float)mouse_x, (float)mouse_y);
                io.AddMousePosEvent((float)mouse_x, (float)mouse_y);
            }
        }

        // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
        // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
        // - [X] GLFW >= 3.3 backend ON WINDOWS ONLY does correctly ignore viewports with the _NoInputs flag.
        // - [!] GLFW <= 3.2 backend CANNOT correctly ignore viewports with the _NoInputs flag, and CANNOT reported Hovered Viewport because of mouse capture.
        //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
        //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
        //       by the backend, and use its flawed heuristic to guess the viewport behind.
        // - [X] GLFW backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
        // FIXME: This is currently only correct on Win32. See what we do below with the WM_NCHITTEST, missing an equivalent for other systems.
        // See https://github.com/glfw/glfw/issues/1236 if you want to help in making this a GLFW feature.
#if GLFW_HAS_MOUSE_PASSTHROUGH || (GLFW_HAS_WINDOW_HOVERED && defined(_WIN32))
        const bool window_no_input = (viewport->Flags & ImGuiViewportFlags_NoInputs) != 0;
#if GLFW_HAS_MOUSE_PASSTHROUGH
        glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, window_no_input);
#endif
        if (glfwGetWindowAttrib(window, GLFW_HOVERED) && !window_no_input)
            mouse_viewport_id = viewport->ID;
#else
        // We cannot use bd->MouseWindow maintained from CursorEnter/Leave callbacks, because it is locked to the window capturing mouse.
#endif
    }

    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
        io.AddMouseViewportEvent(mouse_viewport_id);
}

static void ImGuiImpl_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(bd->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int n = 0; n < platform_io.Viewports.Size; n++)
    {
        GLFWwindow* window = (GLFWwindow*)platform_io.Viewports[n]->PlatformHandle;
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else
        {
            // Show OS mouse cursor
            // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
            glfwSetCursor(window, bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor] : bd->MouseCursors[ImGuiMouseCursor_Arrow]);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

// Update gamepad inputs
static inline float Saturate(float v) { return v < 0.0f ? 0.0f : v  > 1.0f ? 1.0f : v; }

static void ImGuiImpl_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0) // FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs.
        return;

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
#if GLFW_HAS_GAMEPAD_API
    GLFWgamepadstate gamepad;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad))
        return;
#define MAP_BUTTON(KEY_NO, BUTTON_NO, _UNUSED)          do { io.AddKeyEvent(KEY_NO, gamepad.buttons[BUTTON_NO] != 0); } while (0)
#define MAP_ANALOG(KEY_NO, AXIS_NO, _UNUSED, V0, V1)    do { float v = gamepad.axes[AXIS_NO]; v = (v - V0) / (V1 - V0); io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); } while (0)
#else
    int axes_count = 0, buttons_count = 0;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
    const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
    if (axes_count == 0 || buttons_count == 0)
        return;
#define MAP_BUTTON(KEY_NO, _UNUSED, BUTTON_NO)          do { io.AddKeyEvent(KEY_NO, (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS)); } while (0)
#define MAP_ANALOG(KEY_NO, _UNUSED, AXIS_NO, V0, V1)    do { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); } while (0)
#endif
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    MAP_BUTTON(ImGuiKey_GamepadStart, GLFW_GAMEPAD_BUTTON_START, 7);
    MAP_BUTTON(ImGuiKey_GamepadBack, GLFW_GAMEPAD_BUTTON_BACK, 6);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft, GLFW_GAMEPAD_BUTTON_X, 2);     // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceRight, GLFW_GAMEPAD_BUTTON_B, 1);     // Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceUp, GLFW_GAMEPAD_BUTTON_Y, 3);     // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadFaceDown, GLFW_GAMEPAD_BUTTON_A, 0);     // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, 13);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, 11);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp, GLFW_GAMEPAD_BUTTON_DPAD_UP, 10);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, 12);
    MAP_BUTTON(ImGuiKey_GamepadL1, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, 4);
    MAP_BUTTON(ImGuiKey_GamepadR1, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, 5);
    MAP_ANALOG(ImGuiKey_GamepadL2, GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, 4, -0.75f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadR2, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, 5, -0.75f, +1.0f);
    MAP_BUTTON(ImGuiKey_GamepadL3, GLFW_GAMEPAD_BUTTON_LEFT_THUMB, 8);
    MAP_BUTTON(ImGuiKey_GamepadR3, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, 9);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft, GLFW_GAMEPAD_AXIS_LEFT_X, 0, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, GLFW_GAMEPAD_AXIS_LEFT_X, 0, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp, GLFW_GAMEPAD_AXIS_LEFT_Y, 1, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown, GLFW_GAMEPAD_AXIS_LEFT_Y, 1, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft, GLFW_GAMEPAD_AXIS_RIGHT_X, 2, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, GLFW_GAMEPAD_AXIS_RIGHT_X, 2, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp, GLFW_GAMEPAD_AXIS_RIGHT_Y, 3, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown, GLFW_GAMEPAD_AXIS_RIGHT_Y, 3, +0.25f, +1.0f);
#undef MAP_BUTTON
#undef MAP_ANALOG
}

static void ImGuiImpl_WindowNewFrame()
{
    // Resize swap chain?
    //if (g_SwapChainRebuild)
    //{
    //    int width, height;
    //    Window::GetSize(&width, &height);
    //    if (width > 0 && height > 0)
    //    {
    //        ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
    //        ImGui_ImplVulkanH_CreateOrResizeWindow(g_ctx.instance, g_ctx.physicalDevice, g_ctx.device, &g_MainWindowData, g_ctx.queueFamily, g_ctx.allocator, width, height, g_MinImageCount);
    //        g_MainWindowData.FrameIndex = 0;
    //        g_SwapChainRebuild = false;
    //    }
    //}

    ImGuiIO& io = ImGui::GetIO();
    ImGuiImpl_WindowData* bd = ImGuiImpl_WindowGetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplGlfw_InitForXXX()?");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(bd->Window, &w, &h);
    glfwGetFramebufferSize(bd->Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / (float)w, (float)display_h / (float)h);
    if (bd->WantUpdateMonitors)
        ImGuiImpl_UpdateMonitors();

    // Setup time step
    double current_time = glfwGetTime();
    io.DeltaTime = bd->Time > 0.0 ? (float)(current_time - bd->Time) : (float)(1.0f / 60.0f);
    bd->Time = current_time;

    ImGuiImpl_UpdateMouseData();
    ImGuiImpl_UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    ImGuiImpl_UpdateGamepads();
}

// -----------------------------------------------------
// ---------- ImGui Graphics Implementation ------------
// -----------------------------------------------------

#include "Engine/Modules/Graphics.hpp"

struct ImGuiImpl_GraphicsData
{
    GraphicsHandle                Pipeline = INVALID_GRAPHICS_HANDLE;
    GraphicsHandle                FontTexture = INVALID_GRAPHICS_HANDLE;

    //ImGui_ImplVulkan_InitInfo   VulkanInitInfo;
    //VkRenderPass                RenderPass;
    //VkDeviceSize                BufferMemoryAlignment;
    //VkPipelineCreateFlags       PipelineCreateFlags;
    //VkDescriptorSetLayout       DescriptorSetLayout;
    //VkPipelineLayout            PipelineLayout;
    //VkPipeline                  Pipeline;
    //uint32_t                    Subpass;
    //VkShaderModule              ShaderModuleVert;
    //VkShaderModule              ShaderModuleFrag;
    //
    //// Font data
    //VkSampler                   FontSampler;
    //VkDeviceMemory              FontMemory;
    //VkImage                     FontImage;
    //VkImageView                 FontView;
    //VkDescriptorSet             FontDescriptorSet;
    //VkDeviceMemory              UploadBufferMemory;
    //VkBuffer                    UploadBuffer;
    //
    //// Render buffers for main window
    //ImGui_ImplVulkanH_WindowRenderBuffers MainWindowRenderBuffers;
    //
    //ImGuiImpl_GraphicsData()
    //{
    //    memset((void*)this, 0, sizeof(*this));
    //    BufferMemoryAlignment = 256;
    //}
};

static const char* imgui_impl_vshader =
"#version 450 core\n"
"layout(location = 0) in vec2 aPos;\n"
"layout(location = 1) in vec2 aUV;\n"
"layout(location = 2) in vec4 aColor;\n"
"layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;\n"
"out gl_PerVertex{ vec4 gl_Position; };\n"
"layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;\n"
"void main()\n"
"{\n"
"    Out.Color = aColor;\n"
"    Out.UV = aUV;\n"
"    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);\n"
"}";

static const char* imgui_impl_pshader =
"#version 450 core\n"
"layout(location = 0) out vec4 fColor;\n"
"layout(set=0, binding=0) uniform sampler2D sTexture;\n"
"layout(location = 0) in struct { vec4 Color; vec2 UV; } In;\n"
"void main()\n"
"{\n"
"    fColor = In.Color * texture(sTexture, In.UV.st);\n"
"}";

static ImGuiImpl_GraphicsData* ImGuiImpl_GraphicsGetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGuiImpl_GraphicsData*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static bool ImGuiImpl_GraphicsCreateDeviceObjects()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiImpl_GraphicsData* bd = ImGuiImpl_GraphicsGetBackendData();

    if (bd->Pipeline == INVALID_GRAPHICS_HANDLE)
    {
        PipelineInfo pipelineInfo;

        pipelineInfo.numRenderTargets = 1;
        pipelineInfo.renderTargetFormats[0] = Graphics::GetColorBufferFormat();
        pipelineInfo.depthStencilFormat = Graphics::GetDepthBufferFormat();

        pipelineInfo.geomTopology = GeomTopology::TRIANGLES;
        pipelineInfo.faceCull = FaceCull::CCW;
        pipelineInfo.depthEnabled = true;

        ShaderInfo shaderInfo;

        GraphicsHandle vShader;
        shaderInfo.shaderType = ShaderType::VERTEX;
        shaderInfo.source = imgui_impl_vshader;
        vShader = Graphics::CreateShader(shaderInfo);
        pipelineInfo.vertShader = vShader;

        GraphicsHandle pShader;
        shaderInfo.shaderType = ShaderType::PIXEL;
        shaderInfo.source = imgui_impl_pshader;
        pShader = Graphics::CreateShader(shaderInfo);
        pipelineInfo.pixelShader = pShader;

        PipelineConstant constants[] =
        {
            PipelineConstant{ ShaderType::VERTEX, sizeof(float) * 0, sizeof(float) * 4 }
        };
        pipelineInfo.constants = constants;
        pipelineInfo.numConstants = 1;

        SamplerInfo samplerLinearClamp
        {
            //FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
            //TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
        };

        ImmutableSampler immutableSamplers[] =
        {
            ImmutableSampler { ShaderType::PIXEL, "sTexture", samplerLinearClamp }
        };
        pipelineInfo.immutableSamplers = immutableSamplers;
        pipelineInfo.numImmutableSamplers = 1;

        LayoutElement layoutElems[] =
        {
            LayoutElement { 0, 0, 2, ValueType::FLOAT32, false },
            LayoutElement { 1, 0, 2, ValueType::FLOAT32, false },
            LayoutElement { 2, 0, 4, ValueType::UINT8, true }
        };
        pipelineInfo.layoutElements = layoutElems;
        pipelineInfo.numElements = 3;

        bd->Pipeline = Graphics::CreatePipeline(pipelineInfo);

        //BufferInfo cbInfo;
        //cbInfo.strideBytes = sizeof(Mat4);
        //cbInfo.usage = UsageFlags::DYNAMIC;
        //cbInfo.bindFlags = BindFlags::UNIFORM_BUFFER;
        //cbInfo.cpuAccessFlags = CpuAccessFlags::WRITE;
        //
        //m_cbuffer = Graphics::CreateBuffer(cbInfo);
        //
        //Graphics::SetStaticVariable(m_pipeline, ShaderType::VERTEX, "Constants", m_cbuffer);
        //
        ////ShaderResourceBinding srb;
        ////s_pipeline.CreateShaderResourceBinding(&srb, true);
    }

    if (bd->FontTexture == INVALID_GRAPHICS_HANDLE)
    {
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        BufferData data;
        data.pData = pixels;
        data.dataSize = width * height * 4 * sizeof(char);

        TextureInfo info;
        info.width = width;
        info.height = height;
        bd->FontTexture = Graphics::CreateTexture(info, data);
    }

    return true;
}

static bool ImGuiImpl_InitializeGraphics()
{
    //IM_ASSERT(g_FunctionsLoaded && "Need to call ImGui_ImplVulkan_LoadFunctions() if IMGUI_IMPL_VULKAN_NO_PROTOTYPES or VK_NO_PROTOTYPES are set!");

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGuiImpl_GraphicsData* bd = IM_NEW(ImGuiImpl_GraphicsData)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_vulkan";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    //IM_ASSERT(info->Instance != VK_NULL_HANDLE);
    //IM_ASSERT(info->PhysicalDevice != VK_NULL_HANDLE);
    //IM_ASSERT(info->Device != VK_NULL_HANDLE);
    //IM_ASSERT(info->Queue != VK_NULL_HANDLE);
    //IM_ASSERT(info->DescriptorPool != VK_NULL_HANDLE);
    //IM_ASSERT(info->MinImageCount >= 2);
    //IM_ASSERT(info->ImageCount >= info->MinImageCount);
    //IM_ASSERT(render_pass != VK_NULL_HANDLE);

    //bd->VulkanInitInfo = *info;
    //bd->RenderPass = render_pass;
    //bd->Subpass = info->Subpass;

    ImGuiImpl_GraphicsCreateDeviceObjects();

    // Our render function expect RendererUserData to be storing the window render buffer we need (for the main viewport we won't use ->Window)
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    mainViewport->RendererUserData = nullptr;// IM_NEW(ImGui_ImplVulkan_ViewportData)();

    return true;
}

static void ImGuiImpl_RenderDrawData()
{
    //// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    //int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    //int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    //if (fb_width <= 0 || fb_height <= 0)
    //    return;
    //
    //ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    //ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
    //if (pipeline == VK_NULL_HANDLE)
    //    pipeline = bd->Pipeline;
    //
    //// Allocate array to store enough vertex/index buffers. Each unique viewport gets its own storage.
    //ImGui_ImplVulkan_ViewportData* viewport_renderer_data = (ImGui_ImplVulkan_ViewportData*)draw_data->OwnerViewport->RendererUserData;
    //IM_ASSERT(viewport_renderer_data != nullptr);
    //ImGui_ImplVulkanH_WindowRenderBuffers* wrb = &viewport_renderer_data->RenderBuffers;
    //if (wrb->FrameRenderBuffers == nullptr)
    //{
    //    wrb->Index = 0;
    //    wrb->Count = v->ImageCount;
    //    wrb->FrameRenderBuffers = (ImGui_ImplVulkanH_FrameRenderBuffers*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameRenderBuffers) * wrb->Count);
    //    memset(wrb->FrameRenderBuffers, 0, sizeof(ImGui_ImplVulkanH_FrameRenderBuffers) * wrb->Count);
    //}
    //IM_ASSERT(wrb->Count == v->ImageCount);
    //wrb->Index = (wrb->Index + 1) % wrb->Count;
    //ImGui_ImplVulkanH_FrameRenderBuffers* rb = &wrb->FrameRenderBuffers[wrb->Index];
    //
    //if (draw_data->TotalVtxCount > 0)
    //{
    //    // Create or resize the vertex/index buffers
    //    size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    //    size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    //    if (rb->VertexBuffer == VK_NULL_HANDLE || rb->VertexBufferSize < vertex_size)
    //        CreateOrResizeBuffer(rb->VertexBuffer, rb->VertexBufferMemory, rb->VertexBufferSize, vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    //    if (rb->IndexBuffer == VK_NULL_HANDLE || rb->IndexBufferSize < index_size)
    //        CreateOrResizeBuffer(rb->IndexBuffer, rb->IndexBufferMemory, rb->IndexBufferSize, index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    //
    //    // Upload vertex/index data into a single contiguous GPU buffer
    //    ImDrawVert* vtx_dst = nullptr;
    //    ImDrawIdx* idx_dst = nullptr;
    //    VkResult err = vkMapMemory(v->Device, rb->VertexBufferMemory, 0, rb->VertexBufferSize, 0, (void**)(&vtx_dst));
    //    check_vk_result(err);
    //    err = vkMapMemory(v->Device, rb->IndexBufferMemory, 0, rb->IndexBufferSize, 0, (void**)(&idx_dst));
    //    check_vk_result(err);
    //    for (int n = 0; n < draw_data->CmdListsCount; n++)
    //    {
    //        const ImDrawList* cmd_list = draw_data->CmdLists[n];
    //        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
    //        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
    //        vtx_dst += cmd_list->VtxBuffer.Size;
    //        idx_dst += cmd_list->IdxBuffer.Size;
    //    }
    //    VkMappedMemoryRange range[2] = {};
    //    range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    //    range[0].memory = rb->VertexBufferMemory;
    //    range[0].size = VK_WHOLE_SIZE;
    //    range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    //    range[1].memory = rb->IndexBufferMemory;
    //    range[1].size = VK_WHOLE_SIZE;
    //    err = vkFlushMappedMemoryRanges(v->Device, 2, range);
    //    check_vk_result(err);
    //    vkUnmapMemory(v->Device, rb->VertexBufferMemory);
    //    vkUnmapMemory(v->Device, rb->IndexBufferMemory);
    //}
    //
    //// Setup desired Vulkan state
    //ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
    //
    //// Will project scissor/clipping rectangles into framebuffer space
    //ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    //ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
    //
    //// Render command lists
    //// (Because we merged all buffers into a single one, we maintain our own offset into them)
    //int global_vtx_offset = 0;
    //int global_idx_offset = 0;
    //for (int n = 0; n < draw_data->CmdListsCount; n++)
    //{
    //    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    //    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    //    {
    //        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
    //        if (pcmd->UserCallback != nullptr)
    //        {
    //            // User callback, registered via ImDrawList::AddCallback()
    //            // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
    //            if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
    //                ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
    //            else
    //                pcmd->UserCallback(cmd_list, pcmd);
    //        }
    //        else
    //        {
    //            // Project scissor/clipping rectangles into framebuffer space
    //            ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
    //            ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
    //
    //            // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
    //            if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
    //            if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
    //            if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
    //            if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
    //            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
    //                continue;
    //
    //            // Apply scissor/clipping rectangle
    //            VkRect2D scissor;
    //            scissor.offset.x = (int32_t)(clip_min.x);
    //            scissor.offset.y = (int32_t)(clip_min.y);
    //            scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
    //            scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
    //            vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    //
    //            // Bind DescriptorSet with font or user texture
    //            VkDescriptorSet desc_set[1] = { (VkDescriptorSet)pcmd->TextureId };
    //            if (sizeof(ImTextureID) < sizeof(ImU64))
    //            {
    //                // We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
    //                IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->FontDescriptorSet);
    //                desc_set[0] = bd->FontDescriptorSet;
    //            }
    //            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->PipelineLayout, 0, 1, desc_set, 0, nullptr);
    //
    //            // Draw
    //            vkCmdDrawIndexed(command_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
    //        }
    //    }
    //    global_idx_offset += cmd_list->IdxBuffer.Size;
    //    global_vtx_offset += cmd_list->VtxBuffer.Size;
    //}
    //
    //// Note: at this point both vkCmdSetViewport() and vkCmdSetScissor() have been called.
    //// Our last values will leak into user/application rendering IF:
    //// - Your app uses a pipeline with VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR dynamic state
    //// - And you forgot to call vkCmdSetViewport() and vkCmdSetScissor() yourself to explicitly set that state.
    //// If you use VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR you are responsible for setting the values before rendering.
    //// In theory we should aim to backup/restore those values but I am not sure this is possible.
    //// We perform a call to vkCmdSetScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github #4644)
    //VkRect2D scissor = { { 0, 0 }, { (uint32_t)fb_width, (uint32_t)fb_height } };
    //vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

static void ImGuiImpl_FrameRender(int w, int h, ImDrawData* drawData)
{
    //ImGui_ImplVulkanH_Window
    //VkResult err;
    //
    //VkSemaphore image_acquired_semaphore = g_ctx.frameSemaphores[g_ctx.semaphoreIndex].imageAcquiredSemaphore;
    //VkSemaphore render_complete_semaphore = g_ctx.frameSemaphores[g_ctx.semaphoreIndex].renderCompleteSemaphore;
    //err = vkAcquireNextImageKHR(g_ctx.device, g_ctx.swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &g_ctx.frameIndex);
    //if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    //{
    //    g_SwapChainRebuild = true;
    //    return;
    //}
    //check_vk_result(err);
    //
    //VkFrame* fd = &g_ctx.frames[g_ctx.frameIndex];
    //{
    //    err = vkWaitForFences(g_ctx.device, 1, &fd->fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
    //    check_vk_result(err);
    //
    //    err = vkResetFences(g_ctx.device, 1, &fd->fence);
    //    check_vk_result(err);
    //}
    //{
    //    err = vkResetCommandPool(g_ctx.device, fd->commandPool, 0);
    //    check_vk_result(err);
    //    VkCommandBufferBeginInfo info = {};
    //    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //    err = vkBeginCommandBuffer(fd->commandBuffer, &info);
    //    check_vk_result(err);
    //}
    //{
    //    VkRenderPassBeginInfo info = {};
    //    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    //    info.renderPass = g_ctx.renderPass;
    //    info.framebuffer = fd->framebuffer;
    //    info.renderArea.extent.width = w;
    //    info.renderArea.extent.height = h;
    //    info.clearValueCount = 1;
    //    info.pClearValues = &g_ctx.clearValue;
    //    vkCmdBeginRenderPass(fd->commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    //}
    
    // Record dear imgui primitives into command buffer
    //ImGuiImpl_RenderDrawData();// drawData, fd->commandBuffer);
    
    //// Submit command buffer
    //vkCmdEndRenderPass(fd->commandBuffer);
    //{
    //    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //    VkSubmitInfo info = {};
    //    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //    info.waitSemaphoreCount = 1;
    //    info.pWaitSemaphores = &image_acquired_semaphore;
    //    info.pWaitDstStageMask = &wait_stage;
    //    info.commandBufferCount = 1;
    //    info.pCommandBuffers = &fd->commandBuffer;
    //    info.signalSemaphoreCount = 1;
    //    info.pSignalSemaphores = &render_complete_semaphore;
    //
    //    err = vkEndCommandBuffer(fd->commandBuffer);
    //    check_vk_result(err);
    //    err = vkQueueSubmit(g_ctx.queue, 1, &info, fd->fence);
    //    check_vk_result(err);
    //}
}

#endif // IMGUI_CUSTOM_IMPL

/*
// -----------------------------------------------------
// --------------- ImGui Implementation ----------------
// -----------------------------------------------------

#ifndef IMGUI_CUSTOM_IMPL
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#endif

bool ImGuiImpl::Initialize(void* pWindow)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;        // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
#ifdef IMGUI_CUSTOM_IMPL
    // Initialize window
    if (!ImGuiImpl_InitializeWindow(pWindow))
    {
        Log::Error("Failed to initialize ImGui window!");
    }

    // Initialize graphics
    if (!ImGuiImpl_InitializeGraphics())
    {
        Log::Error("Failed to initialize ImGui graphics!");
    }
#else
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)pWindow, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.Instance = g_Instance;
    //init_info.PhysicalDevice = g_PhysicalDevice;
    //init_info.Device = g_Device;
    //init_info.QueueFamily = g_QueueFamily;
    //init_info.Queue = g_Queue;
    //init_info.PipelineCache = g_PipelineCache;
    //init_info.DescriptorPool = g_DescriptorPool;
    //init_info.Subpass = 0;
    //init_info.MinImageCount = g_MinImageCount;
    //init_info.ImageCount = wd->ImageCount;
    //init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    //init_info.Allocator = g_Allocator;
    //init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);
#endif

    //float xs = 0, ys = 0;
    //glfwGetWindowContentScale((GLFWwindow*)pWindow, &xs, &ys);
    //const float uiScale = (xs + ys) / 2.0f;
    //const float fontSize = 14.0f;
    //const float iconSize = 12.0f;
    
    //ImFontConfig config;
    //config.OversampleH = 8;
    //config.OversampleV = 8;
    //config.MergeMode = true;
    //static const ImWchar iconsRanges[] = { 0xf000, 0xf3ff, 0 }; // will not be copied by AddFont* so keep in scope.
    //io.Fonts->AddFontFromFileTTF(File::GetPath("[games]/shared/fonts/DroidSans.ttf").c_str(), fontSize * uiScale, &config, iconsRanges);
    
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    
    // Upload Fonts
    //{
    //    // Use any command queue
    //    VkCommandPool command_pool = g_ctx.frames[g_ctx.frameIndex].commandPool;
    //    VkCommandBuffer command_buffer = g_ctx.frames[g_ctx.frameIndex].commandBuffer;
    //
    //    VkResult err = vkResetCommandPool(g_ctx.device, command_pool, 0);
    //    check_vk_result(err);
    //    VkCommandBufferBeginInfo begin_info = {};
    //    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //    err = vkBeginCommandBuffer(command_buffer, &begin_info);
    //    check_vk_result(err);
    //
    //    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
    //
    //    VkSubmitInfo end_info = {};
    //    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //    end_info.commandBufferCount = 1;
    //    end_info.pCommandBuffers = &command_buffer;
    //    err = vkEndCommandBuffer(command_buffer);
    //    check_vk_result(err);
    //    err = vkQueueSubmit(g_ctx.queue, 1, &end_info, VK_NULL_HANDLE);
    //    check_vk_result(err);
    //
    //    err = vkDeviceWaitIdle(g_ctx.device);
    //    check_vk_result(err);
    //    ImGui_ImplVulkan_DestroyFontUploadObjects();
    //}
    
    const String iniPath = File::GetPath("[save]/imgui.ini");
    static char* g_iniPath = new char[1024];
    strcpy_s(g_iniPath, iniPath.size() + 1, iniPath.c_str());
    io.IniFilename = g_iniPath;

    return true;
}

void ImGuiImpl::Shutdown()
{
    ImGuiImpl_WindowShutdown();
    //ImGuiImpl_GraphicsShutdown();
    
    ImGui::DestroyContext();
}

void ImGuiImpl::NewFrame()
{
    //ImGuiImpl_WindowNewFrame();
    //
    //// Start the Dear ImGui frame
    //ImGui::NewFrame();
}

void ImGuiImpl::EndFrame()
{
    //int w, h;
    //Window::GetSize(&w, &h);
    //
    //ImGui::Render();
    //ImDrawData* drawData = ImGui::GetDrawData();
    //const bool isMinimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);
    ////wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
    ////wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
    ////wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
    ////wd->ClearValue.color.float32[3] = clear_color.w;
    //if (!isMinimized)
    //    ImGuiImpl_FrameRender(w, h, drawData);
    //
    //// Update and Render additional Platform Windows
    //ImGuiIO& io = ImGui::GetIO();
    //if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    //{
    //    ImGui::UpdatePlatformWindows();
    //    ImGui::RenderPlatformWindowsDefault();
    //}
    //
    //// Present Main Platform Window
    ////if (!isMinimized)
    ////    �mGuiImpl_FramePresent();
}
*/