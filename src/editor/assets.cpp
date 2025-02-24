#include <editor/assets.hpp>

#include <engine/memory.hpp>

#include <framework/editor/world_editor.hpp>

BX_EDITOR_MENUITEM_REGISTRATION(ICON_EDITOR"/Windows/Assets", AssetsEditor)
{
	Editor::Get().AddWindow<AssetsEditor>();
}

BX_TYPE_REGISTRATION
{
	rttr::registration::class_<AssetsEditor>("AssetsEditor")
	.constructor()(rttr::policy::ctor::as_std_shared_ptr)
	;

	rttr::type::register_wrapper_converter_for_base_classes<SharedPtr<AssetsEditor>>();
}

AssetsEditor::AssetsEditor()
{
	SetTitle("Assets");
	SetExclusive(true);
	SetPresistent(false);

	GetAssetsEditorInfo();
}

void AssetsEditor::OnGui(EditorApplication& app)
{
	if (ImGui::BeginPopupContextWindow("##AssetsContextMenu"))
	{
		auto& assetsEditorInfo = GetAssetsEditorInfo();
		for (const auto& info : assetsEditorInfo)
		{
			info.onContextMenuGui(app, *this);
		}

		ImGui::EndPopup();
	}
}

static List<AssetEditorInfo> RegisterAssetsEditorInfo()
{
	List<AssetEditorInfo> assetsEditorInfo{};

	auto derivedClasses = rttr::type::get<AssetEditorRegister>().get_derived_classes();
	for (const auto& derivedClass : derivedClasses)
	{
		const auto& derivedClassName = derivedClass.get_name();
		BX_LOGD(Editor, "Registering asset editor info: {}", derivedClassName.data());

		auto registerMethod = derivedClass.get_method("Register");
		if (!registerMethod.is_valid())
		{
			BX_LOGE(Editor, "Register method not found for class: {}", derivedClassName.data());
			continue;
		}

		rttr::instance instance;  // Create a default instance, we are calling a static function
		auto ret = registerMethod.invoke(instance);
		
		AssetEditorInfo info = ret.convert<AssetEditorInfo>();
		assetsEditorInfo.emplace_back(info);
	}

	return assetsEditorInfo;
}

const List<AssetEditorInfo>& AssetsEditor::GetAssetsEditorInfo()
{
	static List<AssetEditorInfo> g_assetsEditorInfo = RegisterAssetsEditorInfo();
	return g_assetsEditorInfo;
}