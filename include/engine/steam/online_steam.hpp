#pragma once

#include <engine/api.hpp>
#include <engine/online.hpp>
#include <engine/guard.hpp>
#include <engine/log.hpp>
#include <engine/file.hpp>

#include <steam/steam_api.h>
#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamclientpublic.h>
#include <steam/isteamgameserver.h>
#include <steam/steam_gameserver.h>

class BX_API OnlineSteam final : public Online
{
    BX_MODULE(OnlineSteam, Online)
    friend class OnlineSteamEditor;

public:
	bool Initialize() override;
	void Shutdown() override;

    void Update() override;

public:
    bool StartClient();
    void StopClient();

    void GetNumberOfCurrentPlayers();

private:
    void AddPlayer(CSteamID steamID);

    void ConnectToServer(CSteamID steamIDGameServer);
    void DisconnectFromServer();

private:
    STEAM_CALLBACK(OnlineSteam, OnGameOverlayActivated, GameOverlayActivated_t);

    void OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t* pCallback, bool bIOFailure);
    CCallResult<OnlineSteam, NumberOfCurrentPlayers_t> m_NumberOfCurrentPlayersCallResult;

    // Multiplayer
    STEAM_CALLBACK(OnlineSteam, OnBeginAuthResponse, ValidateAuthTicketResponse_t, m_callbackBeginAuthResponse);

private:
    List<Player> m_players;
};