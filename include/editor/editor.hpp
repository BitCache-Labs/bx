#pragma once

#include <engine/string.hpp>
#include <engine/memory.hpp>
#include <engine/list.hpp>
#include <engine/function.hpp>
#include <engine/macros.hpp>

#include <editor/editor_application.hpp>

#include <rttr/rttr_enable.h>
#include <rttr/registration.h>

#include <imgui.h>

enum struct EditorTheme
{
	DARK,
	LIGHT,
	GRAY,
	ACRYLIC
};

// So we need to change this to reflect something more like Unity
// Editor<T> is the inspector equivalent, see: https://docs.unity3d.com/Manual/editor-CustomEditors.html
// EditorView is the editor window equivalent, see: https://docs.unity3d.com/Manual/editor-EditorWindows.html
// By this logic Editor will become EditorView, and 

struct EditorMenuItemRegister
{
	RTTR_ENABLE()
};

#define EDITOR_MENUITEM(Path, Class)												\
namespace                                                                           \
{                                                                                   \
    struct Class##MenuItemRegister final : public EditorMenuItemRegister			\
    {                                                                               \
	RTTR_ENABLE(EditorMenuItemRegister)												\
	public:																			\
        Class##MenuItemRegister()													\
        {                                                                           \
			rttr::registration::													\
			class_<Class##MenuItemRegister>(STR(Class##_MenuItemRegister))			\
			.method("Register", Class##MenuItemRegister::Register);					\
        }                                                                           \
		static void Register()														\
		{																			\
			Editor::Get().AddMenuItem(Path, Class::ShowWindow);				\
		}																			\
    };                                                                              \
}                                                                                   \
static const Class##MenuItemRegister g_##Class##MenuItemRegister;

#define EDITOR_MENUBAR(Callback)													\
namespace                                                                           \
{                                                                                   \
    struct EditorMenuBarRegister													\
    {                                                                               \
        EditorMenuBarRegister()														\
        {                                                                           \
			Editor::Get().AddMenuBar(Callback);								\
        }                                                                           \
    };                                                                              \
}                                                                                   \
static const EditorMenuBarRegister g_EditorMenuBarRegister;

template <typename T>
class EditorInspector
{
public:
	static void OnGui(T& obj)
	{
		// TODO: Use RTTR for automatic basic editor.
		// A specialization of this class is needed for custom editor.
		CString<64> str;
		str.format("TODO##{}", typeid(T));
		ImGui::Text(str.c_str());
	}
};

class EditorWindow
{
	RTTR_ENABLE()

public:
	virtual ~EditorWindow() {}
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
	friend class Editor;

	bool m_isOpen{ true };
	bool m_isPersistent{ false };
	bool m_isExclusive{ false };

	CString<64> m_title{ "Editor" };
	ImGuiWindowFlags m_flags{ ImGuiWindowFlags_None };
};

class Editor
{
public:
	static Editor& Get();

	bool Initialize();
	void Shutdown();

	void AddMenuItem(StringView path, Function<void()>&& callback);
	
	template <typename T>
	void AddWindow() { AddWindow(meta::make_unique<T>()); }

	void OnGui(EditorApplication& app);
	void Clear();

private:
	void AddWindow(UniquePtr<EditorWindow> window);

private:
	void RegisterMenuItems();

	void ApplyTheme();
	void PushMenuTheme();
	void PopMenuTheme();

private:
	f32 m_uiScale = 1.0f;
	EditorTheme m_currentTheme{ EditorTheme::ACRYLIC };

	List<Function<void()>> m_menuItems;
	List<UniquePtr<EditorWindow>> m_editorWindows;
};