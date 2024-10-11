#pragma once

#include <bx/bx.hpp>
#include <rttr/rttr_enable.h>

typedef void (*WindowGLProc)(void);

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

	// ImGui calls (tmp solution until imgui is fully done via interface)
	virtual bool InitializeImGui() = 0;
	virtual void ShutdownImGui() = 0;
	virtual void NewFrameImGui() = 0;
	virtual void EndFrameImGui() = 0;
};