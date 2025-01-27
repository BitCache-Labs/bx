#include <engine/steam/online_steam.hpp>

Online& Online::Get()
{
    return OnlineSteam::Get();
}

OnlineSteam& OnlineSteam::Get()
{
    static OnlineSteam instance;
    return instance;
}

extern "C" static void __cdecl SteamAPIDebugTextHook(int nSeverity, const char* pchDebugText)
{
    // if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
    // if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
    LOGD(Online, pchDebugText);

    if (nSeverity >= 1)
    {
        // place to set a breakpoint for catching API errors
        int x = 3;
        (void)x;
    }
}

OnlineSteam::OnlineSteam()
    : m_callbackBeginAuthResponse(this, &OnlineSteam::OnBeginAuthResponse)
{
}

bool OnlineSteam::Initialize()
{
    u32 appId = 480;

#ifdef DEBUG_BUILD
    CString<64> appIdStr{};
    appIdStr.format("{}", appId);
    File::Get().WriteText("steam_appid.txt", appIdStr.c_str());
#endif

    if (SteamAPI_RestartAppIfNecessary(appId))
    {
        LOGD(Online, "Restarting app to launch on Steam.");
        return false;
    }
    
    // Initialize the Steam API
    if (!SteamAPI_Init())
    {
        LOGE(Online, "Failed to initialize Steam API!");
        return false;
    }

    LOGD(Online, "Steam API initialized successfully!");
    
    // Set Steam debug handler
    SteamClient()->SetWarningMessageHook(&SteamAPIDebugTextHook);

    // Ensure that the user has logged into Steam. This will always return true if the game is launched
    // from Steam, but if Steam is at the login prompt when you run your game from the debugger, it
    // will return false.
    if (!SteamUser()->BLoggedOn())
    {
        LOGE(Online, "Steam user is not logged in, a user must be logged in to play this game!");
        return false;
    }

    //const char* pchServerAddress, * pchLobbyID;
    //if (!ParseCommandLine(pchCmdLine, &pchServerAddress, &pchLobbyID))
    //{
    //    // no connect string on process command line. If app was launched via a Steam URL, the extra command line parameters in that URL
    //    // get be retrieved with GetLaunchCommandLine. This way an attacker can't put malicious parameters in the process command line
    //    // which might allow much more functionality then indented.
    //
    //    char szCommandLine[1024] = {};
    //
    //    if (SteamApps()->GetLaunchCommandLine(szCommandLine, sizeof(szCommandLine)) > 0)
    //    {
    //        ParseCommandLine(szCommandLine, &pchServerAddress, &pchLobbyID);
    //    }
    //}

//    if (!SteamInput()->Init(false))
//    {
//        OutputDebugString("SteamInput()->Init failed.\n");
//        Alert("Fatal Error", "SteamInput()->Init failed.\n");
//        return EXIT_FAILURE;
//    }
//    char rgchCWD[1024];
//    if (!_getcwd(rgchCWD, sizeof(rgchCWD)))
//    {
//        strcpy(rgchCWD, ".");
//    }
//
//    char rgchFullPath[1024];
//#if defined(OSX)
//    // hack for now, because we do not have utility functions available for finding the resource path
//    // alternatively we could disable the SteamController init on OS X
//    _snprintf(rgchFullPath, sizeof(rgchFullPath), "%s/steamworksexample.app/Contents/Resources/%s", rgchCWD, "steam_input_manifest.vdf");
//#else
//    _snprintf(rgchFullPath, sizeof(rgchFullPath), "%s\\%s", rgchCWD, "steam_input_manifest.vdf");
//#endif
//
//    SteamInput()->SetInputActionManifestFilePath(rgchFullPath);

    // ------------ Example code --------------------------
    if (!SteamUser() && SteamFriends())
    {
        LOGW(Online, "Failed to retrieve Steam user information!");
    }
    else
    {
        LOGD(Online, "Logged in as: {}", SteamFriends()->GetPersonaName());
    }
    GetNumberOfCurrentPlayers();

    return true;
}

void OnlineSteam::Shutdown()
{
    SteamAPI_Shutdown();
    LOGD(Online, "Steam API shutdown completed.");
}

void OnlineSteam::Update()
{
    //ReceiveNetworkData();

    SteamAPI_RunCallbacks();
}

bool OnlineSteam::StartClient()
{
    if (SteamUser()->BLoggedOn())
    {
        AddPlayer(SteamUser()->GetSteamID());
    }

    // Initialize the peer to peer connection process
    SteamNetworkingUtils()->InitRelayNetworkAccess();

    return true;
}

void OnlineSteam::StopClient()
{
    DisconnectFromServer();
}

void OnlineSteam::AddPlayer(CSteamID steamID)
{
    Player player{};
    player.isLocal = steamID == SteamUser()->GetSteamID();
    player.id = steamID.ConvertToUint64();
    
    m_players.emplace_back(player);
}

void OnlineSteam::ConnectToServer(CSteamID steamIDGameServer)
{
//    if (m_eGameState == k_EClientInLobby && m_steamIDLobby.IsValid())
//    {
//        SteamMatchmaking()->LeaveLobby(m_steamIDLobby);
//    }
//
//    SetGameState(k_EClientGameConnecting);
//
//    m_steamIDGameServerFromBrowser = m_steamIDGameServer = steamIDGameServer;
//
//    SteamNetworkingIdentity identity;
//    identity.SetSteamID(steamIDGameServer);
//
//    m_hConnServer = SteamNetworkingSockets()->ConnectP2P(identity, 0, 0, nullptr);
//    if (m_pVoiceChat)
//        m_pVoiceChat->m_hConnServer = m_hConnServer;
//    if (m_pP2PAuthedGame)
//        m_pP2PAuthedGame->m_hConnServer = m_hConnServer;
//
//    // Update when we last retried the connection, as well as the last packet received time so we won't timeout too soon,
//    // and so we will retry at appropriate intervals if packets drop
//    m_ulLastNetworkDataReceivedTime = m_ulLastConnectionAttemptRetryTime = m_pGameEngine->GetGameTickCount();
}

void OnlineSteam::DisconnectFromServer()
{
//    if (m_eConnectedStatus != k_EClientNotConnected)
//    {
//#ifdef USE_GS_AUTH_API
//        if (m_hAuthTicket != k_HAuthTicketInvalid)
//            SteamUser()->CancelAuthTicket(m_hAuthTicket);
//        m_hAuthTicket = k_HAuthTicketInvalid;
//#else
//        SteamUser()->AdvertiseGame(k_steamIDNil, 0, 0);
//#endif
//
//        // tell steam china duration control system that we are no longer in a match
//        SteamUser()->BSetDurationControlOnlineState(k_EDurationControlOnlineState_Offline);
//
//        m_eConnectedStatus = k_EClientNotConnected;
//    }
//    if (m_pP2PAuthedGame)
//    {
//        m_pP2PAuthedGame->EndGame();
//    }
//
//    if (m_pVoiceChat)
//    {
//        m_pVoiceChat->StopVoiceChat();
//    }
//
//    if (m_hConnServer != k_HSteamNetConnection_Invalid)
//        SteamNetworkingSockets()->CloseConnection(m_hConnServer, k_EDRClientDisconnect, nullptr, false);
//    m_steamIDGameServer = CSteamID();
//    m_steamIDGameServerFromBrowser = CSteamID();
//    m_hConnServer = k_HSteamNetConnection_Invalid;
}

void OnlineSteam::OnGameOverlayActivated(GameOverlayActivated_t* pCallback)
{
    if (pCallback->m_bActive)
        LOGI(Online, "Steam overlay now active");
    else
        LOGI(Online, "Steam overlay now inactive");
}

// Make the asynchronous request to receive the number of current players.
void OnlineSteam::GetNumberOfCurrentPlayers()
{
    LOGD(Online, "Getting Number of Current Players");
    SteamAPICall_t hSteamAPICall = SteamUserStats()->GetNumberOfCurrentPlayers();
    m_NumberOfCurrentPlayersCallResult.Set(hSteamAPICall, this, &OnlineSteam::OnGetNumberOfCurrentPlayers);
}

// Called when SteamUserStats()->GetNumberOfCurrentPlayers() returns asynchronously, after a call to SteamAPI_RunCallbacks().
void OnlineSteam::OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t* pCallback, bool bIOFailure)
{
    if (bIOFailure || !pCallback->m_bSuccess)
    {
        LOGW(Online, "NumberOfCurrentPlayers_t failed!");
        return;
    }

    LOGI(Online, "Number of players currently playing: {}", pCallback->m_cPlayers);
}

void OnlineSteam::OnBeginAuthResponse(ValidateAuthTicketResponse_t* pCallback)
{
    //if (m_steamID == pCallback->m_SteamID)
    //{
    //    CString<128> rgch;
    //    rgch.format("P2P:: Received steam response for account={}", m_steamID.GetAccountID());
    //    LOGD(Online, rgch.c_str());
    //    //m_ulAnswerTime = GetGameTimeInSeconds();
    //    //m_bHaveAnswer = true;
    //    //m_eAuthSessionResponse = pCallback->m_eAuthSessionResponse;
    //}
}