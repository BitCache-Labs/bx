#pragma once

#include "bx/engine/core/type.hpp"

// TODO: remove this class
class Screen
{
public:
	static int GetWidth();
	static int GetHeight();

	static void SetWidth(int width);
	static void SetHeight(int height);
};

ENUM(CursorMode, NORMAL, HIDDEN, DISABLED, CAPTURED);

class Window
{
public:
	static void GetSize(int* width, int* height);
	static void SetCursorMode(CursorMode mode);

private:
	friend class Runtime;
	friend class Module;

	static bool Initialize();
	static void Reload();
	static void Shutdown();

	static bool IsOpen();
	static void PollEvents();
	static void Display();
};