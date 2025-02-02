#include <editor/steam/online_steam.hpp>
#include <engine/steam/online_steam.hpp>

#include <imgui_internal.h>

BX_EDITOR_MENUITEM_REGISTRATION("Modules/Online/Steam", OnlineSteamEditor)
{
	Editor::Get().AddWindow<OnlineSteamEditor>();
}

OnlineSteamEditor::OnlineSteamEditor()
{
	SetTitle("Steam");
	SetExclusive(true);
	SetPresistent(false);
}

void OnlineSteamEditor::OnGui(EditorApplication& app)
{
	static CString<64> g_lobbyName{ "LobbyName" };
	static CString<1024> g_message{};

	bool inLobby = OnlineSteam::Get().InLobby();
	if (inLobby) ImGui::BeginDisabled();
	ImGui::Text("Lobby Name: ");
	ImGui::SameLine();
	ImGui::InputText("##LobbyName", g_lobbyName.data(), g_lobbyName.size());
	ImGui::SameLine();
	if (ImGui::Button("Create"))
		OnlineSteam::Get().CreateLobby(LobbyInfo{ g_lobbyName });

	if (ImGui::Button("Refresh"))
		OnlineSteam::Get().FetchLobbies();

	ImGui::Text("Available Lobbies:");
	const auto& lobbies = OnlineSteam::Get().GetLobbies();

	static SizeType g_selectedLobby = -1;
	for (SizeType i = 0; i < lobbies.size(); ++i)
	{
		const auto& lobby = lobbies[i];
		if (ImGui::Selectable(lobby.name.c_str(), g_selectedLobby == i))
		{
			g_selectedLobby = i;
		}
	}
	if (inLobby) ImGui::EndDisabled();

	bool hasSelection = g_selectedLobby != -1 && g_selectedLobby < lobbies.size();
	if (!hasSelection) ImGui::BeginDisabled();
	if (ImGui::Button("Join"))
		OnlineSteam::Get().JoinLobby(lobbies[g_selectedLobby]);
	if (!hasSelection) ImGui::EndDisabled();

	if (!inLobby) ImGui::BeginDisabled();
	if (ImGui::Button("Leave"))
		OnlineSteam::Get().LeaveLobby();
	
	ImGui::SeparatorText("Chat");
	bool send = ImGui::InputText("##Message", g_message.data(), g_message.size(), ImGuiInputTextFlags_EnterReturnsTrue);
	if (ImGui::Button("Send") || send)
	{
		OnlineSteam::Get().SendMessage(g_message);
		g_message.clear();
	}
	if (!inLobby) ImGui::EndDisabled();
}