// TODO: This should be in bx-engine, its left here until imgui implementation
// can be fully done with the bx-platform window and graphics APIs

#include "bx/platform/imgui.hpp"
#include "bx/platform/file.hpp"

#include <bx/core/macros.hpp>
#include <bx/core/profiler.hpp>

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

    if (!Initialize_Window())
        return false;

    if (!Initialize_Graphics())
        return false;

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
    ImGui::DestroyContext();
}

void ImGuiImpl::NewFrame()
{
    ImGui::NewFrame();
}

void ImGuiImpl::EndFrame()
{
    ImGui::Render();
    EndFrame_Graphics();
    EndFrame_Window();
}