#pragma once

//#include <rttr/rttr_enable.h>
#include <engine/byte_types.hpp>

enum struct ClientStatus { DISCONNECTED, CONNECTED };

enum struct ServerStatus { DISCONNECTED, CONNECTED };

struct Player
{
	bool isLocal{ false };
	u64 id{ 0 };
};

class Online
{
	//RTTR_ENABLE()

public:
	static Online& Get();

public:
	Online() = default;
	virtual ~Online() = default;

	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;

	virtual void Update() = 0;
};