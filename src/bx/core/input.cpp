#include "bx/engine/core/input.hpp"

#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/profiler.hpp"

#ifdef BX_WINDOW_GLFW_BACKEND
#include "bx/engine/modules/window/backend/window_glfw.hpp"
#endif

static GLFWgamepadstate gamepad_state;
static GLFWgamepadstate prev_gamepad_state;

static constexpr int nr_keys = 350;
static bool keys_down[nr_keys];
static bool prev_keys_down[nr_keys];
static bool keys_down_changed[nr_keys];

static constexpr int nr_mousebuttons = 8;
static bool mousebuttons_down[nr_mousebuttons];
static bool prev_mousebuttons_down[nr_mousebuttons];
static bool mousebuttons_down_changed[nr_keys];

static float mousepos[2];
static bool gamepad_connected;

void joystick_callback(int joy, int event) {}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// get the screen-to-game scaling parameters
	//const auto& screen_to_game = xs::configuration::get_scale_to_game(xs::device::get_width(), xs::device::get_height());

	// translate the mouse position to game coordinates
	//xs::configuration::scale_to_game(static_cast<int>(xpos), static_cast<int>(ypos), screen_to_game, mousepos[0], mousepos[1]);

	mousepos[0] = (float)xpos;
	mousepos[1] = (float)ypos;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_RELEASE)
		keys_down_changed[key] = true;
}

void mousebutton_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_RELEASE)
		mousebuttons_down_changed[button] = true;
}

bool Input::Initialize()
{
#ifdef BX_WINDOW_GLFW_BACKEND
	glfwSetJoystickCallback(joystick_callback);
	glfwSetCursorPosCallback(WindowGLFW::GetWindowPtr(), cursor_position_callback);
	glfwSetKeyCallback(WindowGLFW::GetWindowPtr(), key_callback);
	glfwSetMouseButtonCallback(WindowGLFW::GetWindowPtr(), mousebutton_callback);
#endif

	Poll();

	return true;
}

void Input::Reload()
{
}

void Input::Shutdown()
{
#ifdef BX_WINDOW_GLFW_BACKEND
	glfwSetJoystickCallback(NULL);
	glfwSetCursorPosCallback(WindowGLFW::GetWindowPtr(), NULL);
#endif
}

void Input::Poll()
{
	PROFILE_FUNCTION();
	for (int i = 0; i < nr_keys; ++i)
	{
		prev_keys_down[i] = keys_down[i];
		if (keys_down_changed[i])
		{
			keys_down[i] = !keys_down[i];
			keys_down_changed[i] = false;
		}
	}

	for (int i = 0; i < nr_mousebuttons; ++i)
	{
		prev_mousebuttons_down[i] = mousebuttons_down[i];
		if (mousebuttons_down_changed[i])
		{
			mousebuttons_down[i] = !mousebuttons_down[i];
			mousebuttons_down_changed[i] = false;
		}
	}

	prev_gamepad_state = gamepad_state;

	if (glfwJoystickPresent(0) && glfwJoystickIsGamepad(0))
		gamepad_connected = glfwGetGamepadState(0, &gamepad_state);
}

float Input::GetAxis(GamepadAxis axis)
{
	if (!gamepad_connected) return 0.0;

	int axis_id = int(axis);
	BX_ENSURE(axis_id >= 0 && axis_id <= GLFW_GAMEPAD_AXIS_LAST);
	return static_cast<float>(gamepad_state.axes[axis_id]);
}

bool Input::GetButton(GamepadButton button)
{
	if (!gamepad_connected) return false;

	int button_id = int(button);
	BX_ENSURE(button_id >= 0 && button_id <= GLFW_GAMEPAD_BUTTON_LAST);
	return static_cast<bool>(gamepad_state.buttons[button_id]);
}

bool Input::GetButtonOnce(GamepadButton button)
{
	if (!gamepad_connected) return false;

	int button_id = int(button);
	BX_ENSURE(button_id >= 0 && button_id <= GLFW_GAMEPAD_BUTTON_LAST);
	return
		!static_cast<bool>(prev_gamepad_state.buttons[button_id]) &&
		static_cast<bool>(gamepad_state.buttons[button_id]);
}

bool Input::GetKey(Key key)
{
	BX_ENSURE(key >= GLFW_KEY_SPACE && key <= GLFW_KEY_LAST);
	return keys_down[key];
}

bool Input::GetKeyOnce(Key key)
{
	BX_ENSURE(key >= GLFW_KEY_SPACE && key <= GLFW_KEY_LAST);
	return keys_down[key] && !prev_keys_down[key];
}

bool Input::GetMouse()
{
	return true;
}

bool Input::GetMouseButton(MouseButton button)
{
	return mousebuttons_down[int(button)];
}

bool Input::GetMouseButtonOnce(MouseButton button)
{
	return mousebuttons_down[int(button)] && !prev_mousebuttons_down[int(button)];
}

float Input::GetMouseX()
{
	return static_cast<float>(mousepos[0]);
}

float Input::GetMouseY()
{
	return static_cast<float>(mousepos[1]);
}

int Input::GetNumTouches()
{
	return 0;
}

int Input::GetTouchId(int index)
{
	return 0;
}

float Input::GetTouchX(int index)
{
	return 0;
}

float Input::GetTouchY(int index)
{
	return 0;
}

void Input::SetPadVibration(int leftRumble, int rightRumble)
{
	// Unimplemented on the PC
}

void Input::SetPadLightbarColor(float r, float g, float b)
{
	// Unimplemented on the PC (specific dualshock 5 controller mechanic)
}

void Input::ResetPadLightbarColor()
{
	// Unimplemented on the PC (specific dualshock 5 controller mechanic)
}