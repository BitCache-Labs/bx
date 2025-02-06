#include <editor/editor.hpp>
#include <engine/engine.hpp>
#include <engine/window.hpp>
#include <engine/graphics.hpp>
#include <engine/file.hpp>

#include <imgui_internal.h>

BX_TYPE_REGISTRATION
{
	rttr::registration::class_<EditorWindow>("EditorWindow");
}

BX_MODULE_DEFINE(Editor)

bool Editor::Initialize()
{
	BX_LOGD(Editor, "Editor initializing ...");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	static auto ini_filename = File::Get().GetPath("[settings]/imgui.ini");
    io.IniFilename = ini_filename.c_str();

	f32 xs, ys;
	Window::Get().GetContentScale(&xs, &ys);
	m_uiScale = (xs + ys) / 2.0f;

	const float UIScale = m_uiScale;
	const float fontSize = 16.0f;
	const float iconSize = 12.0f;

	ImFontConfig config;
	config.OversampleH = 8;
	config.OversampleV = 8;

	//FilePath textfont_path = File::Get().GetPath("[assets:engine]/free-fonts/Droid/Droid Sans/TTF/DroidSans.ttf");
	FilePath textfont_path = File::Get().GetPath("[assets:engine]/free-fonts/Cousine/TTF/Cousine-Regular.ttf");
	io.Fonts->AddFontFromFileTTF(textfont_path.c_str(), fontSize * UIScale, &config);
	//CString<128> selawkPath = "selawk.ttf";
	//io.Fonts->AddFontFromFileTTF(selawkPath.data(), fontSize * UIScale, &config);

	static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // will not be copied by AddFont* so keep in scope.
	config.MergeMode = true;
	config.OversampleH = 8;
	config.OversampleV = 8;

	FilePath iconfont_path = File::Get().GetPath("[assets:engine]/Font-Awesome/otfs/Font Awesome 6 Free-Solid-900.otf");
	io.Fonts->AddFontFromFileTTF(iconfont_path.data(), iconSize * UIScale, &config, icons_ranges);

	//m_smallFont = io.Fonts->AddFontFromFileTTF(selawkPath.data(), 0.8f * fontSize * UIScale);

	ApplyTheme();

    if (!Window::Get().InitializeImGui())
    {
		BX_LOGE(Editor, "Failed to initialize window imgui!");
        return false;
    }

    if (!Graphics::Get().InitializeImGui())
    {
		BX_LOGE(Editor, "Failed to initialize graphics imgui!");
        return false;
    }

	RegisterMenuItems();

	BX_LOGD(Editor, "Editor initialized successfully.");
    return true;
}

void Editor::Shutdown()
{
	BX_LOGD(Editor, "Editor shutting down ...");

	Clear();
    Graphics::Get().ShutdownImGui();
    Window::Get().ShutdownImGui();
    ImGui::DestroyContext();

	BX_LOGD(Engine, "Editor shutdown complete.");
}

static void MenuItemRecursive(const std::vector<StringView>& parts, std::function<void()>&& callback, SizeType index = 0)
{
	if (index >= parts.size())
		return;

	const bool isLeaf = (index == parts.size() - 1);
	if (isLeaf)
	{
		CString<64> str{ parts[index] };
		if (ImGui::MenuItem(str.c_str()))
			std::move(callback)();
	}
	else
	{
		CString<64> str{ parts[index] };
		if (ImGui::BeginMenu(str.c_str()))
		{
			MenuItemRecursive(parts, std::move(callback), index + 1);
			ImGui::EndMenu();
		}
	}
}

void Editor::AddMenuItem(StringView path, Function<void()>&& callback)
{
	m_menuItems.emplace_back([path, callback = std::move(callback)]() mutable
		{
			const auto parts = File::Get().SplitPath(path);
			if (!parts.empty())
			{
				// If there's only one element, handle it as a special case
				if (parts.size() == 1)
				{
					CString<64> str{ parts[0] };
					if (ImGui::MenuItem(str.c_str()))
						callback();
				}
				else
				{
					MenuItemRecursive(parts, std::move(callback));
				}
			}
		});
}

void Editor::AddWindow(UniquePtr<EditorWindow> window)
{
    if (window->m_isExclusive)
    {
        for (const auto& e : m_editorWindows)
        {
            if (window->m_title == e->m_title)
                return;
        }
    }
	m_pendingEditorWindows.emplace_back(std::move(window));
}

void Editor::OnGui(EditorApplication& app)
{
    Graphics::Get().NewFrameImGui();
    Window::Get().NewFrameImGui();
    ImGui::NewFrame();

	PushMenuTheme();
    if (ImGui::BeginMainMenuBar())
    {
		app.OnMainMenuBar();

        for (const auto& menuItem : m_menuItems)
			menuItem();

        ImGui::EndMainMenuBar();
    }
	PopMenuTheme();

	while (!m_pendingEditorWindows.empty())
	{
		m_editorWindows.push_back(std::move(m_pendingEditorWindows.back()));
		m_pendingEditorWindows.pop_back();
	}

    for (auto it = m_editorWindows.begin(); it != m_editorWindows.end();)
    {
        auto& window = **it;

        bool* open = nullptr;
        if (!window.m_isPersistent)
        {
            open = &window.m_isOpen;
        }

		CString<64> title{};
		if (window.m_isExclusive)
			title = window.m_title;
		else
			title.format("{}##{}", window.m_title.c_str(), window.m_uuid);

        if (ImGui::Begin(title.c_str(), open, window.m_flags))
        {
			window.OnGui(app);
        }
        ImGui::End();

        if (!window.IsOpen())
            it = m_editorWindows.erase(it);
        else
            ++it;
    }

	ImGui::ShowDemoWindow();

    Graphics::Get().EndFrameImGui();
    Window::Get().EndFrameImGui();
}

void Editor::Clear()
{
	m_editorWindows.clear();
}

namespace ImGui
{
	static void StyleEmbraceTheDarkness();
	static void StyleFollowTheLight();
	static void StyleGoGray();
	static void StyleSeeThrough();
}

void Editor::RegisterMenuItems()
{
	// Get the list of all classes derived from EditorMenuItemRegister
	auto derivedClasses = rttr::type::get<EditorMenuItemRegister>().get_derived_classes();
	for (const auto& derivedClass : derivedClasses)
	{
		const auto& derivedClassName = derivedClass.get_name();
		BX_LOGD(Editor, "Registering menu item: {}", derivedClassName.data());

		// Retrieve the "Register" method from the derived class
		auto registerMethod = derivedClass.get_method("Register");
		if (!registerMethod.is_valid())
		{
			BX_LOGE(Editor, "Register method not found for class: {}", derivedClassName.data());
			continue;
		}

		rttr::instance instance;  // Create a default instance, we are calling a static function
		registerMethod.invoke(instance);
	}
}

void Editor::ApplyTheme()
{
	switch (m_currentTheme)
	{
	case EditorTheme::ACRYLIC:
		ImGui::StyleSeeThrough();
		break;
	case EditorTheme::DARK:
		ImGui::StyleEmbraceTheDarkness();
		break;
	case EditorTheme::LIGHT:
		ImGui::StyleFollowTheLight();
		break;
	case EditorTheme::GRAY:
		ImGui::StyleGoGray();
		break;
	default:
		ImGui::StyleColorsDark();
	}
}

void Editor::PushMenuTheme()
{
	switch (m_currentTheme)
	{
	case EditorTheme::ACRYLIC:
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.31f, 0.31f, 0.31f, 0.31f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.23f, 0.23f, 0.00f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		break;
	case EditorTheme::DARK:
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.00f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		break;
	case EditorTheme::LIGHT:
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.00f));
		break;
	case EditorTheme::GRAY:
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.00f));
		break;
	}
}

void Editor::PopMenuTheme()
{
	switch (m_currentTheme)
	{
	case EditorTheme::ACRYLIC:
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(2);
		break;
	case EditorTheme::DARK:
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(1);
		break;
	case EditorTheme::LIGHT:
		ImGui::PopStyleColor(1);
		break;
	case EditorTheme::GRAY:
		ImGui::PopStyleColor(1);
		break;
	}
}

namespace ImGui
{
	static void StyleEmbraceTheDarkness()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
		colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.80f, 0.80f, 0.54f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.00f, 8.00f);
		style.FramePadding = ImVec2(5.00f, 2.00f);
		style.ItemSpacing = ImVec2(6.00f, 6.00f);
		style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
		style.IndentSpacing = 25;
		style.ScrollbarSize = 15;
		style.GrabMinSize = 10;
		style.WindowBorderSize = 1;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 1;
		style.FrameBorderSize = 1;
		style.TabBorderSize = 1;
		style.WindowRounding = 7;
		style.ChildRounding = 4;
		style.FrameRounding = 3;
		style.PopupRounding = 4;
		style.ScrollbarRounding = 9;
		style.GrabRounding = 3;
		style.LogSliderDeadzone = 4;
		style.TabRounding = 4;
		style.FrameBorderSize = 0;
	}

	static void StyleFollowTheLight()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
		colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
		colors[ImGuiCol_Button] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.76f, 0.80f, 0.84f, 0.93f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.60f, 0.73f, 0.88f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowBorderSize = 0;
		style.ChildBorderSize = 0;
		style.PopupBorderSize = 0;
		style.FrameBorderSize = 0;
		style.TabBorderSize = 0;
	}

	static void StyleGoGray()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
		colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.24f, 0.24f, 0.24f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.54f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.22f, 0.22f, 0.22f, 0.54f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_Button] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.10f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.50f, 0.50f, 0.50f, 0.5f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.00f, 8.00f);
		style.FramePadding = ImVec2(5.00f, 2.00f);
		style.ItemSpacing = ImVec2(6.00f, 6.00f);
		style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
		style.IndentSpacing = 25;
		style.ScrollbarSize = 10;
		style.GrabMinSize = 10;
		style.WindowBorderSize = 1;
		style.ChildBorderSize = 0;
		style.PopupBorderSize = 0;
		style.FrameBorderSize = 0;
		style.TabBorderSize = 1;
		style.WindowRounding = 7;
		style.ChildRounding = 4;
		style.FrameRounding = 3;
		style.PopupRounding = 4;
		style.ScrollbarRounding = 9;
		style.GrabRounding = 3;
		style.LogSliderDeadzone = 4;
		style.TabRounding = 4;
		style.FrameBorderSize = 0;
	}

	static void StyleSeeThrough()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(1.00f, 1.00f, 1.00f, 0.59f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
		colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.78f, 0.78f, 0.78f, 0.39f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.78f, 0.78f, 0.78f, 0.59f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.13f, 0.12f, 0.15f, 0.59f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.12f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.78f, 0.78f, 0.78f, 0.39f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.78f, 0.78f, 0.78f, 0.59f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.59f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.59f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.78f);
		colors[ImGuiCol_Button] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.59f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.12f, 0.15f, 0.59f);
		colors[ImGuiCol_Header] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);
		colors[ImGuiCol_Separator] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.59f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.78f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.59f);
		colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.13f, 0.15f, 0.04f);
		colors[ImGuiCol_TabSelected] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
		colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_TabDimmed] = ImVec4(0.13f, 0.13f, 0.15f, 0.04f);
		colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.14f, 0.14f, 0.20f);
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 0.00f, 0.78f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 0.00f, 0.78f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.83f, 0.86f, 0.33f, 1.00f);
		colors[ImGuiCol_NavCursor] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.33f, 0.67f, 0.86f, 0.78f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.33f, 0.67f, 0.86f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);

		// Style settings (unchanged)
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.00f, 8.00f);
		style.FramePadding = ImVec2(5.00f, 2.00f);
		style.ItemSpacing = ImVec2(6.00f, 6.00f);
		style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		style.ScrollbarSize = 15;
		style.GrabMinSize = 10;
		style.WindowBorderSize = 1;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 1;
		style.FrameBorderSize = 1;
		style.TabBorderSize = 1;
		style.WindowRounding = 7;
		style.ChildRounding = 4;
		style.FrameRounding = 3;
		style.PopupRounding = 4;
		style.ScrollbarRounding = 9;
		style.GrabRounding = 3;
		style.LogSliderDeadzone = 4;
		style.TabRounding = 4;
	}
}