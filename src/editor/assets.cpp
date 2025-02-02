#include <editor/assets.hpp>
#include <framework/editor/world_editor.hpp>

BX_EDITOR_MENUITEM_REGISTRATION("Window/Assets", AssetsEditor)
{
	Editor::Get().AddWindow<AssetsEditor>();
}

AssetsEditor::AssetsEditor()
{
	SetTitle("Assets");
	SetExclusive(true);
	SetPresistent(false);
}

void AssetsEditor::OnGui(EditorApplication& app)
{
	if (ImGui::Button("Create World"))
		Editor::Get().AddWindow<WorldEditor>((SceneManager&)app);
}