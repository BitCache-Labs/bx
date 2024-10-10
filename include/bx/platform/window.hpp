#pragma once

#include <bx/bx.hpp>
#include <rttr/rttr_enable.h>

using WindowGLProc = void(*)(void);

enum struct BX_API CursorMode
{
	NORMAL,
	HIDDEN,
	DISABLED,
	CAPTURED
};

class BX_API Window
{
	RTTR_ENABLE()

public:
	static Window& Get();

public:
	Window() = default;
	virtual ~Window() = default;

	virtual bool Initialize() = 0;
	virtual void Reload() = 0;
	virtual void Shutdown() = 0;

	virtual bool IsOpen() = 0;
	virtual void PollEvents() = 0;
	virtual void Display() = 0;

	virtual void GetSize(int* width, int* height) = 0;
	virtual void SetCursorMode(CursorMode mode) = 0;

	virtual WindowGLProc GetProcAddress(const char* name) = 0;
};