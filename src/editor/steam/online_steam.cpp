#include <editor/editor.hpp>
//#include <engine/steam/online_steam.hpp>

class OnlineSteamEditor final : public EditorView
{
	RTTR_ENABLE()

public:
	OnlineSteamEditor();
	void OnGui() override;
};

EDITOR_MENU("Modules/Online/Steam", []() { EditorManager::Get().AddView(meta::make_unique<OnlineSteamEditor>()); })

OnlineSteamEditor::OnlineSteamEditor()
{
	SetTitle("Steam");
	SetExclusive(true);
	SetPresistent(false);
}

void OnlineSteamEditor::OnGui()
{
}