#pragma once

#include <engine/api.hpp>
#include <engine/string.hpp>
#include <engine/memory.hpp>
#include <engine/list.hpp>
#include <engine/function.hpp>
#include <engine/macros.hpp>
#include <engine/module.hpp>
#include <engine/uuid.hpp>

#include <editor/application.hpp>

#include <imgui.h>

enum struct BX_API EditorTheme
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

struct BX_API EditorMenuItemRegister
{
	BX_TYPE(EditorMenuItemRegister)
};

#define BX_EDITOR_MENUITEM_REGISTRATION(Path, Class)								\
static void Class##MenuItemShowWindow();											\
namespace                                                                           \
{                                                                                   \
    struct BX_API Class##MenuItemRegister final : public EditorMenuItemRegister		\
    {                                                                               \
	BX_TYPE(Class##MenuItemRegister, EditorMenuItemRegister)						\
	public:																			\
        Class##MenuItemRegister()													\
        {                                                                           \
			rttr::registration::													\
			class_<Class##MenuItemRegister>(BX_STR(Class##_MenuItemRegister))			\
			.method("Register", Class##MenuItemRegister::Register);					\
        }                                                                           \
		static void Register()														\
		{																			\
			Editor::Get().AddMenuItem(Path, Class##MenuItemShowWindow);				\
		}																			\
    };                                                                              \
}                                                                                   \
static const Class##MenuItemRegister g_##Class##MenuItemRegister;					\
static void Class##MenuItemShowWindow()

template <typename T>
class BX_API EditorInspector
{
	BX_TYPE(EditorInspector)

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

class BX_API EditorWindow
{
	BX_TYPE(EditorWindow)

public:
	virtual ~EditorWindow() {}
	virtual void OnGui(EditorApplication& app) = 0;

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
	UUID m_uuid{ GenUUID::MakeUUID() };
};

class BX_API Editor
{
	BX_MODULE(Editor)

public:
	bool Initialize();
	void Shutdown();

	void AddMenuItem(StringView path, Function<void()>&& callback);
	
	template <typename T, typename... Args>
	void AddWindow(Args&&... args)
	{
		static_assert(std::is_base_of<EditorWindow, T>::value, "T must derive from EditorWindow");
		AddWindow(meta::make_unique<T>(std::forward<Args>(args)...));
	}

	void OnGui(EditorApplication& app);
	void Clear();

	inline f32 GetUIScale() const { return m_uiScale; }

private:
	void AddWindow(UniquePtr<EditorWindow> window);

private:
	void RegisterMenuItems();

	void ApplyTheme();

public:
	void PushMenuTheme();
	void PopMenuTheme();

private:
	f32 m_uiScale{ 1.0f };
	EditorTheme m_currentTheme{ EditorTheme::ACRYLIC };

	List<Function<void()>> m_menuItems{};
	List<UniquePtr<EditorWindow>> m_editorWindows{};
	List<UniquePtr<EditorWindow>> m_pendingEditorWindows{};
};