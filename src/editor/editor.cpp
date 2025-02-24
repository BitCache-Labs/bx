#include <editor/editor.hpp>

#include <engine/engine.hpp>
#include <engine/version.hpp>
#include <engine/window.hpp>
#include <engine/graphics.hpp>
#include <engine/file.hpp>
#include <engine/enum.hpp>
#include <engine/guard.hpp>
#include <engine/serial.hpp>

#include <editor/assets.hpp>

BX_TYPE_REGISTRATION
{
	rttr::registration::class_<EditorWindow>("EditorWindow")
	.property("uuid", &EditorWindow::m_uuid)
	;
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
	FilePath textfont_path = File::Get().GetPath("[assets]/free-fonts/Cousine/TTF/Cousine-Regular.ttf");
	io.Fonts->AddFontFromFileTTF(textfont_path.c_str(), fontSize * UIScale, &config);
	//CString<128> selawkPath = "selawk.ttf";
	//io.Fonts->AddFontFromFileTTF(selawkPath.data(), fontSize * UIScale, &config);

	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 }; // will not be copied by AddFont* so keep in scope.
	config.MergeMode = true;
	config.OversampleH = 8;
	config.OversampleV = 8;

	FilePath iconfont_path = File::Get().GetPath("[assets]/Font-Awesome/otfs/Font Awesome 6 Free-Solid-900.otf");
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

	BX_LOGD(Editor, "Loading open editors ...");
	{
		auto stream = File::Get().InputStream("[settings]/editors.json");
		if (stream.is_open())
		{
			cereal::JSONInputArchive archive(stream);
			BX_TRYCATCH(archive(cereal::make_nvp("editors", m_pendingEditorWindows)));
		}
	}
	BX_LOGD(Editor, "Load complete.");

	BX_LOGD(Editor, "Editor initialized successfully.");
    return true;
}

void Editor::Shutdown()
{
	BX_LOGD(Editor, "Editor shutting down ...");

	BX_LOGD(Editor, "Saving open editors ...");
	{
		auto stream = File::Get().OutputStream("[settings]/editors.json");
		if (stream.is_open())
		{
			cereal::JSONOutputArchive archive(stream);
			BX_TRYCATCH(archive(cereal::make_nvp("editors", m_editorWindows)));
		}
	}
	BX_LOGD(Editor, "Save complete.");

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

void Editor::AddWindow(SharedPtr<EditorWindow> window)
{
    if (window->m_isExclusive)
    {
        for (const auto& e : m_editorWindows)
        {
            if (window->m_title == e->m_title)
                return;
        }
    }
	else
	{
		window->m_uuid = GenUUID::MakeUUID();
	}

	m_pendingEditorWindows.emplace_back(std::move(window));
}

void Editor::OnGui(EditorApplication& app)
{
    Graphics::Get().NewFrameImGui();
    Window::Get().NewFrameImGui();
    ImGui::NewFrame();

	ApplyTheme();

	OnMainMenuBarGui(app);

	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

	// Run editor windows GUI
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

		bool isFocused = (ImGui::GetCurrentContext()->NavWindow &&
			ImGui::GetCurrentContext()->NavWindow->Name &&
			strcmp(ImGui::GetCurrentContext()->NavWindow->Name, title.c_str()) == 0);

		//if (isFocused)
		//	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.3f, 1.0f)); // Custom color

        if (ImGui::Begin(title.c_str(), open, window.m_flags))
        {
			window.OnGui(app);
        }

		//if (isFocused)
		//	ImGui::PopStyleColor();

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

void Editor::OnMainMenuBarGui(EditorApplication& app)
{
	ImGui::PushMenuBarTheme();
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::Button(ICON_FA_ADJUST))
		{
			m_currentTheme = (m_currentTheme + 1) % Enum::count<EditorTheme>();
		}
		ImGui::Tooltip("Theme");

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

		ImGui::Text("| bx v%s |", BX_VERSION_STR);
		ImGui::Tooltip("Version");

		if (ImGui::Button(ICON_FA_QUESTION_CIRCLE))
		{
			// TODO: About dialog
		}
		ImGui::Tooltip("About");

		ImGui::PopStyleColor(); // Pop text color

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

		if (ImGui::Button(ICON_FA_BUG))
		{
			//show_settings = !show_settings;
		}
		ImGui::Tooltip("Debug");

		if (ImGui::Button(ICON_FA_CHART_PIE))
		{
			m_showProfiler = !m_showProfiler;
		}
		ImGui::Tooltip("Profiler");

		if (ImGui::Button(ICON_FA_COG))
		{
			m_showSettings = !m_showSettings;
		}
		ImGui::Tooltip("Settings");

		if (ImGui::Button(ICON_FA_TERMINAL))
		{
			m_showConsole = !m_showConsole;
		}
		ImGui::Tooltip("Console");

		if (ImGui::Button(ICON_FA_EDIT))
		{
			//show_inspector = !show_inspector;
		}
		ImGui::Tooltip("Inspector");

		if (ImGui::Button(ICON_FA_IMAGES))
		{
			//show_assets = !show_assets;
			Editor::Get().AddWindow<AssetsEditor>();
		}
		ImGui::Tooltip("Assets");

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

		app.OnMainMenuBarGui();

		for (const auto& menuItem : m_menuItems)
			menuItem();

		ImGui::EndMainMenuBar();
	}
	ImGui::PopMenuBarTheme();
}