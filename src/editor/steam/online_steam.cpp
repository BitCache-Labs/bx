#include <editor/steam/online_steam.hpp>
#include <engine/steam/online_steam.hpp>

EDITOR_MENUITEM("Modules/Online/Steam", OnlineSteamEditor)
void OnlineSteamEditor::ShowWindow()
{
	Editor::Get().AddWindow<OnlineSteamEditor>();
}

OnlineSteamEditor::OnlineSteamEditor()
{
	SetTitle("Steam");
	SetExclusive(true);
	SetPresistent(false);
}

void OnlineSteamEditor::OnGui()
{
	if (ImGui::Button("Start Client"))
		OnlineSteam::Get().StartClient();

	if (ImGui::Button("Stop Client"))
		OnlineSteam::Get().StopClient();
}