#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/list.hpp>

#include <editor/editor.hpp>

// TODO: Rename to AssetEditor?
class AssetsEditor;

struct AssetEditorInfo
{
	using OnContextMenuGui = void(*)(EditorApplication&, AssetsEditor&);
	using OnImport = void(*)(EditorApplication&, AssetsEditor&);

	OnContextMenuGui onContextMenuGui{};
	OnImport onImport{};
};

struct BX_API AssetEditorRegister
{
	BX_TYPE(AssetEditorRegister)
};

#define BX_ASSET_REGISTRATION(Class)													\
static AssetEditorInfo Class##AssetEditorInfoRegister();								\
namespace																				\
{																						\
    struct BX_API Class##AssetEditorRegister final : public AssetEditorRegister			\
    {																					\
	BX_TYPE(Class##AssetRegAssetEditorRegisterster, AssetEditorRegister)				\
	public:																				\
        Class##AssetEditorRegister()													\
        {																				\
			rttr::registration::														\
			class_<Class##AssetEditorRegister>(BX_STR(Class##_AssetEditorRegister))		\
			.method("Register", Class##AssetEditorRegister::Register);					\
        }																				\
		static AssetEditorInfo Register()												\
		{																				\
			return Class##AssetEditorInfoRegister();									\
		}																				\
    };																					\
}																						\
static const Class##AssetEditorRegister g_##Class##AssetEditorRegister;					\
static AssetEditorInfo Class##AssetEditorInfoRegister()

class BX_API AssetsEditor final
	: public EditorWindow
{
	BX_TYPE(AssetsEditor, EditorWindow)

public:
	AssetsEditor();
	void OnGui(EditorApplication& app) override;

private:
	static const List<AssetEditorInfo>& GetAssetsEditorInfo();
};