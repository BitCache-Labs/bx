#pragma once

#include <engine/api.hpp>
#include <engine/window.hpp>
#include <engine/log.hpp>
#include <engine/enum.hpp>
#include <engine/macros.hpp>

#include <GLFW/glfw3.h>

#include <stdlib.h>

class BX_API WindowGLFW final
	: public Window
{
	BX_MODULE(WindowGLFW, Window)
	friend class WindowGLFWEditor;

public:
	bool Initialize() override;
	void Shutdown() override;

	bool IsOpen() override;
	void PollEvents() override;
	void Display() override;

	void GetSize(i32* width, i32* height) override;
	void GetContentScale(f32* xscale, f32* yscale) override;

	void SetCursorMode(CursorMode mode) override;

	f32 GetAxis(GamepadAxis axis) override;
	bool GetButton(GamepadButton button) override;
	bool GetButtonOnce(GamepadButton button) override;
	bool GetKey(Key key) override;
	bool GetKeyOnce(Key key) override;
	bool GetMouseButton(MouseButton button) override;
	bool GetMouseButtonOnce(MouseButton button) override;
	f32 GetMouseX() override;
	f32 GetMouseY() override;
	i32 GetNumTouches() override;
	i32 GetTouchId(i32 index) override;
	f32 GetTouchX(i32 index) override;
	f32 GetTouchY(i32 index) override;
	void SetPadVibration(i32 leftRumble, i32 rightRumble) override;
	void SetPadLightbarColor(f32 r, f32 g, f32 b) override;
	void ResetPadLightbarColor() override;

	WindowGLProc GetProcAddress(const char* name) override;

#ifdef EDITOR_BUILD
	bool InitializeImGui() override;
	void ShutdownImGui() override;
	void NewFrameImGui() override;
	void EndFrameImGui() override;
#endif

public:
	GLFWwindow* GetWindowPtr() const;

private:
	static void glfw_error_callback(i32 i, const char* c);
	static void glfw_window_size_callback(GLFWwindow* window, i32 width, i32 height);
	static void glfw_joystick_callback(i32 joy, i32 event);
	static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	static void glfw_key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
	static void glfw_mousebutton_callback(GLFWwindow* window, i32 button, i32 action, i32 mods);

private:
	GLFWwindow* m_pWindow = nullptr;
};