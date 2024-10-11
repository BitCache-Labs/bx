#pragma once

#include <bx/bx.hpp>
#include <rttr/rttr_enable.h>

class BX_API Network
{
	RTTR_ENABLE()

public:
	static Network& Get();

public:
	virtual bool Initialize() = 0;
	virtual void Reload() = 0;
	virtual void Shutdown() = 0;
};