#pragma once

#include <engine/api.hpp>
#include <engine/online.hpp>
#include <engine/guard.hpp>
#include <engine/log.hpp>
#include <engine/file.hpp>
#include <engine/list.hpp>

#include <steam/steam_api.h>
#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamclientpublic.h>

#include <steam/isteamgameserver.h>
#include <steam/steam_gameserver.h>

class BX_API SteamCallback
{
    BX_NOCOPY(SteamCallback)

public:
    SteamCallback();
    ~SteamCallback();

private:
    STEAM_CALLBACK(SteamCallback, OnGameOverlayActivated, GameOverlayActivated_t);

    STEAM_CALLBACK(SteamCallback, OnBeginAuthResponse, ValidateAuthTicketResponse_t, m_callbackBeginAuthResponse);

    STEAM_CALLBACK(SteamCallback, OnLobbyCreated, LobbyCreated_t, m_callbackLobbyCreated);
    STEAM_CALLBACK(SteamCallback, OnLobbyMatchList, LobbyMatchList_t, m_callbackLobbyMatchList);
    STEAM_CALLBACK(SteamCallback, OnLobbyEntered, LobbyEnter_t, m_callbackLobbyEntered);

    STEAM_CALLBACK(SteamCallback, OnP2PSessionRequest, P2PSessionRequest_t, m_callbackP2PSessionRequest);
    STEAM_CALLBACK(SteamCallback, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_callbackP2PSessionConnectFail);
};

class BX_API OnlineSteam final
    : public Online
{
    BX_MODULE(OnlineSteam, Online)
    friend class OnlineSteamEditor;
    friend class SteamCallback;

public:
	bool Initialize() override;
	void Shutdown() override;

    void Update() override;

public:
    /*async*/ void GetNumberOfCurrentPlayers();

    /*async*/ void CreateLobby(const LobbyInfo& info);
    /*async*/ void JoinLobby(const Lobby& lobby);
    /*async*/ void LeaveLobby();
    /*async*/ void FetchLobbies();

    /*async*/ void AddPlayer(const PlayerInfo& info);

    const bool InLobby() const { return m_currentLobby.IsValid() && m_currentLobby.IsLobby(); }
    const List<Lobby>& GetLobbies() const { return m_lobbyList; }

    void SendMessage(const StringView message);

private:
    void ReceiveMessages();

private:
    void OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t* pCallback, bool bIOFailure);
    CCallResult<OnlineSteam, NumberOfCurrentPlayers_t> m_NumberOfCurrentPlayersCallResult;

private:
    SteamCallback m_steamCallback{};

    List<Lobby> m_lobbyList{};
    CSteamID m_currentLobby{};
    bool m_isHosting{ false };

    bool m_isLobbyPendingCreate{ false };
    LobbyInfo m_pendingLobbyInfo{};

    CSteamID m_peerID{};
};