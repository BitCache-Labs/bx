#pragma once

//#include <rttr/rttr_enable.h>

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