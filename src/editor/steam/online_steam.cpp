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
	static CString<64> g_lobbyName{ "LobbyName" };
	static CString<1024> g_message{};

	ImGui::Text("Lobby Name: ");
	ImGui::SameLine();
	ImGui::InputText("##LobbyName", g_lobbyName.data(), g_lobbyName.size());

	if (ImGui::Button("Create Lobby"))
		OnlineSteam::Get().CreateLobby(LobbyInfo{ g_lobbyName });

	if (ImGui::Button("Refresh Lobbies"))
		OnlineSteam::Get().FetchLobbies();

	ImGui::Text("Available Lobbies:");
	const auto& lobbies = OnlineSteam::Get().GetLobbies();
	for (size_t i = 0; i < lobbies.size(); ++i)
	{
		std::string lobbyName = SteamMatchmaking()->GetLobbyData(lobbies[i], "name");
		if (ImGui::Button(lobbyName.empty() ? ("Lobby " + std::to_string(i)).c_str() : lobbyName.c_str()))
		{
			OnlineSteam::Get().JoinLobby(lobbies[i]);
		}
	}

	ImGui::SeparatorText("Chat");
	ImGui::InputText("##Message", g_message.data(), g_message.size());
	if (ImGui::Button("Send"))
		OnlineSteam::Get().SendMessage(g_message);
}