#pragma once

#include <engine/api.hpp>
#include <engine/module.hpp>
#include <engine/byte_types.hpp>
#include <engine/string.hpp>
#include <engine/log.hpp>

LOG_CHANNEL(Online)

struct BX_API LobbyInfo
{
	CString<64> name{};
};

struct BX_API Lobby
{
	u64 id{ 0 };
	CString<64> name{};
};

struct BX_API PlayerInfo
{
	CString<64> name{};
	bool isLocal{ true };
};

struct BX_API Player
{
	u64 id{ 0 };
	CString<64> name{};
};

class BX_API Online
{
	BX_MODULE_INTERFACE(Online)

public:
	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;

	virtual void Update() = 0;
};