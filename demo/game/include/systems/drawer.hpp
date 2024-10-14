#pragma once

#include <bx/bx.hpp>
#include <bx/engine/ecs.hpp>
#include <rttr/rttr_enable.h>

class GAME_API Drawer final : public System<Drawer>
{
	RTTR_ENABLE(SystemBase)

public:
	Drawer();

private:
	void Initialize() override;
	void Shutdown() override;

	void Update() override;
	void Render() override;
};