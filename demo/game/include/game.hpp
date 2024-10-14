#pragma once

#include <bx/bx.hpp>
#include <bx/engine/plugin.hpp>

class GAME_API Game final : public Plugin
{
	RTTR_ENABLE(Plugin)
public:
	Game();

	bool Initialize() override;
	void Reload() override;
	void Shutdown() override;
};