#include <engine/online.hpp>
#include <engine/guard.hpp>
#include <engine/log.hpp>
#include <engine/file.hpp>

#include <steam/steam_api.h>

// Client
#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

// Server
#include <steam/isteamnetworkingsockets.h>
#include <steam/steamclientpublic.h>
#include <steam/isteamgameserver.h>
#include <steam/steam_gameserver.h>

class OnlineSteam final : public Online
{
    //RTTR_ENABLE(Online)
    SINGLETON(OnlineSteam)

public:
    static OnlineSteam& Get();

public:
	bool Initialize() override;
	void Shutdown() override;

    void Update() override;

    bool StartClient();
    void StopClient();

    bool StartServer();
    void StopServer();

    void GetNumberOfCurrentPlayers();

private:
    void ConnectToServer(CSteamID steamIDGameServer);
    void DisconnectFromServer();

private:
    STEAM_CALLBACK(OnlineSteam, OnGameOverlayActivated, GameOverlayActivated_t);

    void OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t* pCallback, bool bIOFailure);
    CCallResult<OnlineSteam, NumberOfCurrentPlayers_t> m_NumberOfCurrentPlayersCallResult;

private:
    CSteamID m_SteamIDLocalUser{};

    // Server
    
    // Socket to listen for new connections on 
    HSteamListenSocket m_hListenSocket;

    // Poll group used to receive messages from all clients at once
    HSteamNetPollGroup m_hNetPollGroup;
};

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
    SteamAPI_RunCallbacks();
}

bool OnlineSteam::StartClient()
{
    if (SteamUser()->BLoggedOn())
    {
        m_SteamIDLocalUser = SteamUser()->GetSteamID();
        //m_eGameState = k_EClientGameMenu;
    }

//    g_pSpaceWarClient = this;
//    m_pGameEngine = pGameEngine;
//    m_uPlayerWhoWonGame = 0;
//    m_ulStateTransitionTime = m_pGameEngine->GetGameTickCount();
//    m_ulLastNetworkDataReceivedTime = 0;
//    m_pServer = NULL;
//    m_uPlayerShipIndex = 0;
//    m_eConnectedStatus = k_EClientNotConnected;
//    m_bTransitionedGameState = true;
//    m_rgchErrorText[0] = 0;
//    m_unServerIP = 0;
//    m_usServerPort = 0;
//    m_ulPingSentTime = 0;
//    m_bSentWebOpen = false;
//    m_hConnServer = k_HSteamNetConnection_Invalid;

    // Initialize the peer to peer connection process
    SteamNetworkingUtils()->InitRelayNetworkAccess();

//    for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
//    {
//        m_rguPlayerScores[i] = 0;
//        m_rgpShips[i] = NULL;
//    }
//
//    // Seed random num generator
//    srand((uint32)time(NULL));
//
//    m_hHUDFont = pGameEngine->HCreateFont(HUD_FONT_HEIGHT, FW_BOLD, false, "Arial");
//    if (!m_hHUDFont)
//        OutputDebugString("HUD font was not created properly, text won't draw\n");
//
//    m_hInstructionsFont = pGameEngine->HCreateFont(INSTRUCTIONS_FONT_HEIGHT, FW_BOLD, false, "Arial");
//    if (!m_hInstructionsFont)
//        OutputDebugString("instruction font was not created properly, text won't draw\n");
//
//    m_hInGameStoreFont = pGameEngine->HCreateFont(INSTRUCTIONS_FONT_HEIGHT, FW_BOLD, false, "Courier New");
//    if (!m_hInGameStoreFont)
//        OutputDebugString("in-game store font was not created properly, text won't draw\n");
//
//    // Initialize starfield
//    m_pStarField = new CStarField(pGameEngine);
//
//    // Initialize main menu
//    m_pMainMenu = new CMainMenu(pGameEngine);
//
//    // Initialize connecting menu
//    m_pConnectingMenu = new CConnectingMenu(pGameEngine);
//
//    // Initialize pause menu
//    m_pQuitMenu = new CQuitMenu(pGameEngine);
//
//    // Initialize sun
//    m_pSun = new CSun(pGameEngine);
//
//    m_nNumWorkshopItems = 0;
//    for (uint32 i = 0; i < MAX_WORKSHOP_ITEMS; ++i)
//    {
//        m_rgpWorkshopItems[i] = NULL;
//    }
//
//    // initialize P2P auth engine
//    m_pP2PAuthedGame = new CP2PAuthedGame(m_pGameEngine);
//
//    // Create matchmaking menus
//    m_pServerBrowser = new CServerBrowser(m_pGameEngine);
//    m_pLobbyBrowser = new CLobbyBrowser(m_pGameEngine);
//    m_pLobby = new CLobby(m_pGameEngine);
//
//
//    // Init stats
//    m_pStatsAndAchievements = new CStatsAndAchievements(pGameEngine);
//    m_pLeaderboards = new CLeaderboards(pGameEngine);
//    m_pFriendsList = new CFriendsList(pGameEngine);
//    m_pMusicPlayer = new CMusicPlayer(pGameEngine);
//    m_pClanChatRoom = new CClanChatRoom(pGameEngine);
//
//    // Remote Play session list
//    m_pRemotePlayList = new CRemotePlayList(pGameEngine);
//
//    // Remote Storage page
//    m_pRemoteStorage = new CRemoteStorage(pGameEngine);
//
//    // P2P voice chat 
//    m_pVoiceChat = new CVoiceChat(pGameEngine);
//
//    // HTML Surface page
//    m_pHTMLSurface = new CHTMLSurface(pGameEngine);
//
//    // in-game store
//    m_pItemStore = new CItemStore(pGameEngine);
//    m_pItemStore->LoadItemsWithPrices();
//
//    m_pOverlayExamples = new COverlayExamples(pGameEngine);
//
//    LoadWorkshopItems();

    return true;
}

void OnlineSteam::StopClient()
{
    DisconnectFromServer();
//
//    if (m_pP2PAuthedGame)
//    {
//        m_pP2PAuthedGame->EndGame();
//        delete m_pP2PAuthedGame;
//        m_pP2PAuthedGame = NULL;
//    }
//
//    if (m_pServer)
//    {
//        delete m_pServer;
//        m_pServer = NULL;
//    }
//
//    if (m_pStarField)
//        delete m_pStarField;
//
//    if (m_pMainMenu)
//        delete m_pMainMenu;
//
//    if (m_pConnectingMenu)
//        delete m_pConnectingMenu;
//
//    if (m_pQuitMenu)
//        delete m_pQuitMenu;
//
//    if (m_pSun)
//        delete m_pSun;
//
//    if (m_pStatsAndAchievements)
//        delete m_pStatsAndAchievements;
//
//    if (m_pServerBrowser)
//        delete m_pServerBrowser;
//
//    if (m_pVoiceChat)
//        delete m_pVoiceChat;
//
//    if (m_pHTMLSurface)
//        delete m_pHTMLSurface;
//
//    for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
//    {
//        if (m_rgpShips[i])
//        {
//            delete m_rgpShips[i];
//            m_rgpShips[i] = NULL;
//        }
//    }
//
//    for (uint32 i = 0; i < MAX_WORKSHOP_ITEMS; ++i)
//    {
//        if (m_rgpWorkshopItems[i])
//        {
//            delete m_rgpWorkshopItems[i];
//            m_rgpWorkshopItems[i] = NULL;
//        }
//    }
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

// TODO: Remove these defines
#define INADDR_ANY (unsigned long)0x00000000
#define SPACEWAR_MASTER_SERVER_UPDATER_PORT 27016
#define SPACEWAR_SERVER_PORT 27015
#define SPACEWAR_SERVER_VERSION "1.0.0.0"

bool OnlineSteam::StartServer()
{
//    m_bConnectedToSteam = false;

    const char* pchGameDir = "spacewar";
    uint32 unIP = INADDR_ANY;
    uint16 usMasterServerUpdaterPort = SPACEWAR_MASTER_SERVER_UPDATER_PORT;

//#ifdef USE_GS_AUTH_API
//    EServerMode eMode = eServerModeAuthenticationAndSecure;
//#else
    // Don't let Steam do authentication
    EServerMode eMode = eServerModeNoAuthentication;
//#endif
    // Initialize the SteamGameServer interface, we tell it some info about us, and we request support
    // for both Authentication (making sure users own games) and secure mode, VAC running in our game
    // and kicking users who are VAC banned

    // !FIXME! We need a way to pass the dedicated server flag here!

    if (!SteamGameServer_Init(unIP, SPACEWAR_SERVER_PORT, usMasterServerUpdaterPort, eMode, SPACEWAR_SERVER_VERSION))
    {
        LOGE(Online, "SteamGameServer_Init call failed");
    }

    if (SteamGameServer())
    {

        // Set the "game dir".
        // This is currently required for all games.  However, soon we will be
        // using the AppID for most purposes, and this string will only be needed
        // for mods.  it may not be changed after the server has logged on
        SteamGameServer()->SetModDir(pchGameDir);

        // These fields are currently required, but will go away soon.
        // See their documentation for more info
        SteamGameServer()->SetProduct("SteamworksExample");
        SteamGameServer()->SetGameDescription("Steamworks Example");

        // We don't support specators in our game.
        // .... but if we did:
        //SteamGameServer()->SetSpectatorPort( ... );
        //SteamGameServer()->SetSpectatorServerName( ... );

        // Initiate Anonymous logon.
        // Coming soon: Logging into authenticated, persistent game server account
        SteamGameServer()->LogOnAnonymous();

        // Initialize the peer to peer connection process.  This is not required, but we do it
        // because we cannot accept connections until this initialization completes, and so we
        // want to start it as soon as possible.
        SteamNetworkingUtils()->InitRelayNetworkAccess();

        // We want to actively update the master server with our presence so players can
        // find us via the steam matchmaking/server browser interfaces
//#ifdef USE_GS_AUTH_API
//        SteamGameServer()->SetAdvertiseServerActive(true);
//#endif
    }
    else
    {
        LOGE(Online, "SteamGameServer() interface is invalid\n");
    }
//
//    m_uPlayerCount = 0;
//    m_pGameEngine = pGameEngine;
//    m_eGameState = k_EServerWaitingForPlayers;
//
//    for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
//    {
//        m_rguPlayerScores[i] = 0;
//        m_rgpShips[i] = NULL;
//    }
//
//    // No one has won
//    m_uPlayerWhoWonGame = 0;
//    m_ulStateTransitionTime = m_pGameEngine->GetGameTickCount();
//    m_ulLastServerUpdateTick = 0;
//
//    // zero the client connection data
//    memset(&m_rgClientData, 0, sizeof(m_rgClientData));
//    memset(&m_rgPendingClientData, 0, sizeof(m_rgPendingClientData));
//
//    // Seed random num generator
//    srand((uint32)time(NULL));
//
//    // Initialize sun
//    m_pSun = new CSun(pGameEngine);
//
//    // Initialize ships
//    ResetPlayerShips();

    // create the listen socket for listening for players connecting
    m_hListenSocket = SteamGameServerNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);

    // create the poll group
    m_hNetPollGroup = SteamGameServerNetworkingSockets()->CreatePollGroup();

    return true;
}

void OnlineSteam::StopServer()
{
    //delete m_pSun;
    //
    //for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
    //{
    //    if (m_rgpShips[i])
    //    {
    //        // Tell this client we are exiting
    //        MsgServerExiting_t msg;
    //        BSendDataToClient(i, (char*)&msg, sizeof(msg));
    //
    //        delete m_rgpShips[i];
    //        m_rgpShips[i] = NULL;
    //    }
    //}

    SteamGameServerNetworkingSockets()->CloseListenSocket(m_hListenSocket);
    SteamGameServerNetworkingSockets()->DestroyPollGroup(m_hNetPollGroup);

    // Disconnect from the steam servers
    SteamGameServer()->LogOff();

    // release our reference to the steam client library
    SteamGameServer_Shutdown();
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