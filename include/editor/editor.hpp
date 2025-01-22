#pragma once

#include <engine/string.hpp>
#include <engine/memory.hpp>
#include <engine/list.hpp>
#include <engine/function.hpp>
#include <engine/macros.hpp>

#include <rttr/rttr_enable.h>
#include <imgui.h>

// So we need to change this to reflect something more like Unity
// Editor<T> is the inspector equivalent, see: https://docs.unity3d.com/Manual/editor-CustomEditors.html
// EditorView is the editor window equivalent, see: https://docs.unity3d.com/Manual/editor-EditorWindows.html
// By this logic Editor will become EditorView, and 

#define EDITOR_MENU(Path, Callback)													\
namespace                                                                           \
{                                                                                   \
    struct EditorMenuRegister														\
    {                                                                               \
        EditorMenuRegister()														\
        {                                                                           \
			EditorManager::Get().AddMenuItem(Path, Callback);						\
        }                                                                           \
    };                                                                              \
}                                                                                   \
static const EditorMenuRegister g_EditorMenuRegister;

enum struct EditorTheme
{
	DARK,
	LIGHT,
	GRAY,
	ACRYLIC
};

template <typename T>
class Editor
{
public:
	static void OnInspectGui(T& obj)
	{
		// TODO: Use RTTR for automatic basic editor.
		// A specialization of this class is needed for custom editor.
		CString<64> str;
		str.format("TODO##{}", typeid(T));
		ImGui::Text(str.c_str());
	}
};

class EditorView
{
public:
	virtual ~EditorView() {}
	virtual void OnGui() = 0;

	bool IsOpen() const { return m_isOpen; }
	void SetPresistent(bool isPersistent) { m_isPersistent = isPersistent; }
	void SetExclusive(bool isExclusive) { m_isExclusive = isExclusive; }
	void Close() { if (!m_isPersistent) m_isOpen = false; }
	
	const char* GetTitle() const { return m_title; }
	void SetTitle(const char* title) { m_title = title; }

	ImGuiWindowFlags GetFlags() const { return m_flags; }
	void SetFlags(ImGuiWindowFlags flags) { m_flags = flags; }

private:
	friend class EditorManager;

	bool m_isOpen{ true };
	bool m_isPersistent{ false };
	bool m_isExclusive{ false };

	CString<64> m_title{ "Editor" };
	ImGuiWindowFlags m_flags{ ImGuiWindowFlags_None };
};

using MenuBarOnGui = void(*)();

class EditorManager
{
public:
	static EditorManager& Get();

	bool Initialize();
	void Shutdown();

	void AddMenuBar(Function<void()>&& callback);
	void AddMenuItem(StringView path, Function<void()>&& callback);
	void AddView(UniquePtr<EditorView> view);
	void OnGui();
	void Clear();

private:
	void ApplyTheme();
	void PushMenuTheme();
	void PopMenuTheme();

private:
	f32 m_uiScale = 1.0f;
	EditorTheme m_currentTheme{ EditorTheme::ACRYLIC };

	List<Function<void()>> m_menuBars;
	List<UniquePtr<EditorView>> m_editorViews;
};