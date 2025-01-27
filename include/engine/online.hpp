#pragma once

#include <engine/api.hpp>
#include <engine/module.hpp>
#include <engine/byte_types.hpp>
#include <engine/string.hpp>

struct BX_API LobbyInfo
{
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