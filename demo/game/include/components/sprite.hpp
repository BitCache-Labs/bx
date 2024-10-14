#pragma once

#include <bx/bx.hpp>
#include <bx/engine/ecs.hpp>
#include <rttr/rttr_enable.h>

class GAME_API Sprite final : public Component<Sprite>
{
	RTTR_ENABLE(ComponentBase)

public:
	Sprite();
};