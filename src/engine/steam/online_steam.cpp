#include <engine/steam/online_steam.hpp>

BX_MODULE_DEFINE(OnlineSteam)
BX_MODULE_DEFINE_INTERFACE(Online, OnlineSteam)

static CSteamID ConvertId(u64 id)
{
    CSteamID steamId{};
    steamId.SetFromUint64(id);
    return steamId;
}

SteamCallback::SteamCallback()
    : m_callbackBeginAuthResponse(this, &SteamCallback::OnBeginAuthResponse)
    , m_callbackLobbyCreated(this, &SteamCallback::OnLobbyCreated)
    , m_callbackLobbyMatchList(this, &SteamCallback::OnLobbyMatchList)
    , m_callbackLobbyEntered(this, &SteamCallback::OnLobbyEntered)
    , m_callbackP2PSessionRequest(this, &SteamCallback::OnP2PSessionRequest)
    , m_callbackP2PSessionConnectFail(this, &SteamCallback::OnP2PSessionConnectFail)
{
}

SteamCallback::~SteamCallback()
{
}

void SteamCallback::OnGameOverlayActivated(GameOverlayActivated_t* pCallback)
{
    if (pCallback->m_bActive)
        BX_LOGI(Online, "Steam overlay now active");
    else
        BX_LOGI(Online, "Steam overlay now inactive");
}

void SteamCallback::OnBeginAuthResponse(ValidateAuthTicketResponse_t* pCallback)
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

void SteamCallback::OnLobbyCreated(LobbyCreated_t* pCallback)
{
    auto& ctx = OnlineSteam::Get();

    if (pCallback->m_eResult == k_EResultOK)
    {
        BX_LOGI(Online, "Lobby created successfully. Lobby ID: {}", pCallback->m_ulSteamIDLobby);
        ctx.m_currentLobby = CSteamID(pCallback->m_ulSteamIDLobby);
        ctx.m_isHosting = true;

        SteamMatchmaking()->SetLobbyData(ctx.m_currentLobby, "name", ctx.m_pendingLobbyInfo.name.c_str());
        SteamMatchmaking()->SetLobbyData(ctx.m_currentLobby, "version", "1.0");
        SteamMatchmaking()->SetLobbyData(ctx.m_currentLobby, "app", "skypi");
    }
    else
    {
        BX_LOGE(Online, "Failed to create lobby. Error: {}", (int)pCallback->m_eResult);
    }

    ctx.m_isLobbyPendingCreate = false;
}

void SteamCallback::OnLobbyMatchList(LobbyMatchList_t* pCallback)
{
    auto& ctx = OnlineSteam::Get();

    ctx.m_lobbyList.clear();
    for (uint32 i = 0; i < pCallback->m_nLobbiesMatching; ++i)
    {
        CSteamID lobbyId = SteamMatchmaking()->GetLobbyByIndex(i);

        Lobby lobby{};
        lobby.id = lobbyId.ConvertToUint64();
        lobby.name = SteamMatchmaking()->GetLobbyData(lobbyId, "name");
        ctx.m_lobbyList.push_back(lobby);
    }
    BX_LOGI(Online, "Found {} lobbies.", ctx.m_lobbyList.size());
}

void SteamCallback::OnLobbyEntered(LobbyEnter_t* pCallback)
{
    auto& ctx = OnlineSteam::Get();

    if (pCallback->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
    {
        ctx.m_currentLobby = CSteamID(pCallback->m_ulSteamIDLobby);

        if (SteamMatchmaking()->GetLobbyOwner(ctx.m_currentLobby) != SteamUser()->GetSteamID())
        {
            ctx.m_peerID = SteamMatchmaking()->GetLobbyOwner(ctx.m_currentLobby);
            BX_LOGI(Online, "Joined lobby ID {}. Host ID: {}", ctx.m_currentLobby.ConvertToUint64(), ctx.m_peerID.ConvertToUint64());
        }
        else
        {
            BX_LOGI(Online, "Lobby ID {} created successfully. Waiting for players...", ctx.m_currentLobby.ConvertToUint64());
        }
    }
    else
    {
        BX_LOGE(Online, "Failed to join lobby.");
    }
}

void SteamCallback::OnP2PSessionRequest(P2PSessionRequest_t* pCallback)
{
    auto& ctx = OnlineSteam::Get();

    if (pCallback->m_steamIDRemote.IsValid())
    {
        SteamNetworking()->AcceptP2PSessionWithUser(pCallback->m_steamIDRemote);
        ctx.m_peerID = pCallback->m_steamIDRemote;
        BX_LOGI(Online, "Accepted P2P session request from: {}", ctx.m_peerID.ConvertToUint64());
    }
}

void SteamCallback::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pCallback)
{
    auto& ctx = OnlineSteam::Get();
    BX_LOGE(Online, "P2P session with {} failed. Error code: {}", pCallback->m_steamIDRemote.ConvertToUint64(), pCallback->m_eP2PSessionError);
}

extern "C" static void __cdecl SteamAPIDebugTextHook(int nSeverity, const char* pchDebugText)
{
    // if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
    // if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
    BX_LOGD(Online, pchDebugText);

    if (nSeverity >= 1)
    {
        // place to set a breakpoint for catching API errors
        int x = 3;
        (void)x;
    }
}

bool OnlineSteam::Initialize()
{
    // TODO: Get App ID from transient settings, the user client must set this on app startup not on disk!
    u32 appId = 480;

#if defined(DEBUG_BUILD) || defined(EDITOR_BUILD)
    if (!File::Get().Exists("steam_appid.txt"))
    {
        CString<64> appIdStr{};
        appIdStr.format("{}", 480);
        File::Get().WriteText("steam_appid.txt", appIdStr.c_str());
    }
    else
    {
        auto appIdStr = File::Get().ReadText("steam_appid.txt");
        appId = std::atoi(appIdStr.c_str());
    }
#endif

    if (SteamAPI_RestartAppIfNecessary(appId))
    {
        BX_LOGD(Online, "Restarting app to launch on Steam.");
        return false;
    }
    
    // Initialize the Steam API
    if (!SteamAPI_Init())
    {
        BX_LOGE(Online, "Failed to initialize Steam API!");
        return false;
    }

    BX_LOGD(Online, "Steam API initialized successfully!");
    
    // Set Steam debug handler
    SteamClient()->SetWarningMessageHook(&SteamAPIDebugTextHook);

    // Ensure that the user has logged into Steam. This will always return true if the game is launched
    // from Steam, but if Steam is at the login prompt when you run your game from the debugger, it
    // will return false.
    if (!SteamUser()->BLoggedOn())
    {
        BX_LOGE(Online, "Steam user is not logged in, a user must be logged in to play this game!");
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
        BX_LOGW(Online, "Failed to retrieve Steam user information!");
    }
    else
    {
        BX_LOGD(Online, "Logged in as: {}", SteamFriends()->GetPersonaName());
    }
    GetNumberOfCurrentPlayers();

    return true;
}

void OnlineSteam::Shutdown()
{
    SteamAPI_Shutdown();
    BX_LOGD(Online, "Steam API shutdown completed.");
}

void OnlineSteam::Update()
{
    //ReceiveNetworkData();

    SteamAPI_RunCallbacks();

    ReceiveMessages();
}

// Make the asynchronous request to receive the number of current players.
void OnlineSteam::GetNumberOfCurrentPlayers()
{
    BX_LOGD(Online, "Getting Number of Current Players");
    SteamAPICall_t hSteamAPICall = SteamUserStats()->GetNumberOfCurrentPlayers();
    m_NumberOfCurrentPlayersCallResult.Set(hSteamAPICall, this, &OnlineSteam::OnGetNumberOfCurrentPlayers);
}

// Called when SteamUserStats()->GetNumberOfCurrentPlayers() returns asynchronously, after a call to SteamAPI_RunCallbacks().
void OnlineSteam::OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t* pCallback, bool bIOFailure)
{
    if (bIOFailure || !pCallback->m_bSuccess)
    {
        BX_LOGW(Online, "NumberOfCurrentPlayers_t failed!");
        return;
    }

    BX_LOGI(Online, "Number of players currently playing: {}", pCallback->m_cPlayers);
}

void OnlineSteam::CreateLobby(const LobbyInfo& info)
{
    m_isLobbyPendingCreate = true;
    m_pendingLobbyInfo = info;

    SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 4);
}

void OnlineSteam::JoinLobby(const Lobby& lobby)
{
    SteamMatchmaking()->JoinLobby(ConvertId(lobby.id));
}

void OnlineSteam::LeaveLobby()
{
    if (m_currentLobby.IsValid())
    {
        BX_LOGI(Online, "Leaving lobby with ID: {}", m_currentLobby.ConvertToUint64());
        SteamMatchmaking()->LeaveLobby(m_currentLobby);
        m_currentLobby = CSteamID(); // Reset the current lobby ID
    }
    else
    {
        BX_LOGE(Online, "No active lobby to leave.");
    }
}

void OnlineSteam::FetchLobbies()
{
    SteamMatchmaking()->AddRequestLobbyListStringFilter("app", "skypi", k_ELobbyComparisonEqual);
    SteamMatchmaking()->AddRequestLobbyListStringFilter("version", "1.0", k_ELobbyComparisonEqual);
    SteamMatchmaking()->AddRequestLobbyListResultCountFilter(10);
    SteamMatchmaking()->RequestLobbyList();
}

void OnlineSteam::SendMessage(const StringView message)
{
    if (!m_peerID.IsValid())
    {
        BX_LOGE(Online, "No peer connected to send messages.");
        return;
    }

    if (SteamNetworking()->SendP2PPacket(m_peerID, message.data(), message.size(), k_EP2PSendReliable))
    {
        BX_LOGI(Online, "{}: {}", SteamFriends()->GetPersonaName(), message);
    }
    else
    {
        BX_LOGE(Online, "Failed to send message.");
    }
}

void OnlineSteam::ReceiveMessages()
{
    CString<1025> buffer{};
    u32 messageSize{ 0 };
    CSteamID sender{};

    while (SteamNetworking()->IsP2PPacketAvailable(&messageSize))
    {
        if (SteamNetworking()->ReadP2PPacket(buffer.data(), sizeof(buffer), &messageSize, &sender))
        {
            StringView message(buffer, messageSize);
            BX_LOGI(Online, "{}: {}", SteamFriends()->GetFriendPersonaName(sender), message);
        }
    }
}