#pragma once

#include <bx/bx.hpp>

#include <rttr/rttr_enable.h>

/// <summary>
/// An enum listing all possible gamepad buttons with digital input values.
/// This is the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further mapping.
/// </summary>
enum struct BX_API GamepadButton
{
	BUTTON_SOUTH = 0,
	BUTTON_EAST = 1,
	BUTTON_WEST = 2,
	BUTTON_NORTH = 3,

	SHOULDER_LEFT = 4,
	SHOULDER_RIGHT = 5,

	BUTTON_SELECT = 6,
	BUTTON_START = 7,

	// Button 8 is not used

	STICK_LEFT = 9,
	STICK_RIGHT = 10,

	DPAD_UP = 11,
	DPAD_RIGHT = 12,
	DPAD_DOWN = 13,
	DPAD_LEFT = 14
};

/// <summary>
/// An enum listing all possible gamepad axes with analog input values.
/// This is the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further mapping.
/// </summary>
enum struct BX_API GamepadAxis
{
	/// Represents the horizontal axis of the left gamepad stick, with an analog input value between -1 (left) and 1 (right).
	STICK_LEFT_X = 0,
	/// Represents the vertical axis of the left gamepad stick, with an analog input value between -1 (down) and 1 (up).
	STICK_LEFT_Y = 1,
	/// Represents the horizontal axis of the right gamepad stick, with an analog input value between -1 (left) and 1 (right).
	STICK_RIGHT_X = 2,
	/// Represents the vertical axis of the right gamepad stick, with an analog input value between -1 (down) and 1 (up).
	STICK_RIGHT_Y = 3,
	/// Represents the left trigger of a gamepad, with an analog input value between 0 (not pressed) and 1 (fully pressed).
	TRIGGER_LEFT = 4,
	/// Represents the right trigger of a gamepad, with an analog input value between 0 (not pressed) and 1 (fully pressed).
	TRIGGER_RIGHT = 5
};

enum struct BX_API MouseButton
{
	MOUSE_BUTTON_LEFT = 0,
	MOUSE_BUTTON_RIGHT = 1,
	MOUSE_BUTTON_MIDDLE = 2
};

// TODO: add more keys
enum struct BX_API Key
{
	RIGHT = 262,
	LEFT = 263,
	DOWN = 264,
	UP = 265,
	SPACE = 32,
	ESCAPE = 256,
	ENTER = 257,

	A = 65,
	B = 66,
	C = 67,
	D = 68,
	E = 69,
	F = 70,
	G = 71,
	H = 72,
	I = 73,
	J = 74,
	K = 75,
	L = 76,
	M = 77,
	N = 78,
	O = 79,
	P = 80,
	Q = 81,
	R = 82,
	S = 83,
	T = 84,
	U = 85,
	V = 86,
	W = 87,
	X = 88,
	Y = 89,
	Z = 90
};

class BX_API Input
{
	RTTR_ENABLE()

public:
	static Input& Get();

public:
	virtual bool Initialize() = 0;
	virtual void Reload() = 0;
	virtual void Shutdown() = 0;

	virtual void PollEvents() = 0;

	/// <summary>
	/// Returns the current floating-point value of a given gamepad axis.
	/// </summary>
	/// <param name="button">The ID of the axis to check.</param>
	/// <returns>The current value of the given axis. For the possible ranges per axis, see the documentation of xs::input::gamepad_axis.</returns>
	virtual float GetAxis(GamepadAxis axis) = 0;

	/// <summary>
	/// Checks and returns whether a given gamepad button is currently being held down.
	/// </summary>
	/// <param name="button">The ID of the button to check.</param>
	/// <returns>true if the given button is being held down; false otherwise.</returns>
	virtual bool GetButton(GamepadButton button) = 0;

	/// <summary>
	/// Checks and returns whether a given gamepad button is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="button">The ID of the button to check.</param>
	/// <returns>true if the given button is being pressed in this frame and not in the previous; false otherwise.</returns>
	virtual bool GetButtonOnce(GamepadButton button) = 0;

	/// <summary>
	/// Checks and returns whether a given keyboard key is currently being held down.
	/// </summary>
	/// <param name="key">The keycode of the key to check.</param>
	/// <returns>true if the given key is being held down; false otherwise.</returns>
	virtual bool GetKey(Key key) = 0;

	/// <summary>
	/// Checks and returns whether a given keyboard key is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="key">The keycode of the key to check.</param>
	/// <returns>true if the given key is being pressed in this frame and not in the previous; false otherwise.</returns>
	virtual bool GetKeyOnce(Key key) = 0;

	/// <summary>
	/// Checks and returns whether mouse input is currently available.
	/// </summary>
	virtual bool GetMouse() = 0;

	/// <summary>
	/// Checks and returns whether a given mouse button is currently being held down.
	/// </summary>
	/// <param name="button">The keycode of the mouse button to check.</param>
	/// <returns>true if the given button is being held down; false otherwise.</returns>
	virtual bool GetMouseButton(MouseButton button) = 0;

	/// <summary>
	/// Checks and returns whether a given mouse button is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="button">The keycode of the mouse button to check.</param>
	/// <returns>true if the given button is being pressed in this frame and not in the previous; false otherwise.</returns>
	virtual bool GetMouseButtonOnce(MouseButton button) = 0;

	/// <summary>
	/// Gets the current X coordinate of the mouse position in game coordinates.
	/// </summary>
	virtual float GetMouseX() = 0;

	/// <summary>
	/// Gets the current Y coordinate of the mouse position in game coordinates.
	/// </summary>
	virtual float GetMouseY() = 0;

	/// <summary>
	/// Gets the current numbers of point where the touchscreen is being touched.
	/// </summary>
	virtual int GetNumTouches() = 0;

	/// <summary>
	/// Gets the unique ID corresponding to a touch array index, or -1 if that touch does not exist.
	/// This can be used to track touches of the same finger across multiple frames.
	/// The array index may change per frame, but the ID will not.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
	virtual int GetTouchId(int index) = 0;

	/// <summary>
	/// Gets the current X coordinate (in game coordinates) of the touch position at the given array index.
	/// Returns 0.0 if the given touch does not exist.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
	virtual float GetTouchX(int index) = 0;

	/// <summary>
	/// Gets the current Y coordinate (in game coordinates) of the touch position at the given array index.
	/// Returns 0.0 if the given touch does not exist.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
	virtual float GetTouchY(int index) = 0;

	/// <summary>
	/// Turns on vibration for the left and right motor if the used controller supports this
	/// </summary>
	/// <param name="lightRumble">Value between 0 (off) and 1 (max rotation) for the left motor</param>
	/// <param name="heavyRumble">Value between 0 (off) and 1 (max rotation) for the right motor</param>
	virtual void SetPadVibration(int leftRumble, int rightRumble) = 0;

	/// <summary>
	/// Sets the color value for the lightbar on Dualshock controllers (4 and up)
	/// <para> Currently only implemented for the PS5 and dualshock 4+ controllers</para>
	/// </summary>
	/// <param name="r, g, b"> The new coulour values in RGB format ranging between 0 and 255</param>
	virtual void SetPadLightbarColor(float r, float g, float b) = 0;

	/// <summary>
	/// Turns off the light bar
	/// <para> Currently only implemented for the PS5 and dualshock 4+ controllers</para> 
	/// </summary>
	virtual void ResetPadLightbarColor() = 0;
};