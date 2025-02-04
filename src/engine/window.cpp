#include <engine/window.hpp>
#include <engine/script.hpp>

static const StringView g_windowSrc = R"(
foreign class GamepadButton {
    construct new(i) {}

    foreign static south
    foreign static east
    foreign static west
    foreign static north
    foreign static shoulderLeft
    foreign static shoulderRight
    foreign static select
    foreign static start
    foreign static leftStickPress
    foreign static rightStickPress
    foreign static dPadUp
    foreign static dPadRight
    foreign static dPadDown
    foreign static dPadLeft
}

foreign class GamepadAxis {
    construct new(i) {}

    foreign static leftStickX
    foreign static leftStickY
    foreign static rightStickX
    foreign static rightStickY
    foreign static leftTrigger
    foreign static rightTrigger
}

foreign class MouseButton {
    construct new(i) {}

    foreign static left
    foreign static right
    foreign static middle
}

foreign class Key {
    construct new(i) {}

    foreign static right
    foreign static left
    foreign static down
    foreign static up
    foreign static space
    foreign static escape
    foreign static enter

    foreign static a
    foreign static b
    foreign static c
    foreign static d
    foreign static e
    foreign static f
    foreign static g
    foreign static h
    foreign static i
    foreign static j
    foreign static k
    foreign static l
    foreign static m
    foreign static n
    foreign static o
    foreign static p
    foreign static q
    foreign static r
    foreign static s
    foreign static t
    foreign static u
    foreign static v
    foreign static w
    foreign static x
    foreign static y
    foreign static z

    // TODO: add more keys
}

class Window {
    foreign static getAxis(axis)
    foreign static getButton(button)
    foreign static getButtonOnce(button)
    
    foreign static getKey(key)
    foreign static getKeyOnce(key)
    
    //foreign static getMouse()
    foreign static getMouseButton(button)
    foreign static getMouseButtonOnce(button)
    foreign static getMouseX()
    foreign static getMouseY()
    
    foreign static getNumTouches()
    foreign static getTouchId(index)
    foreign static getTouchX(index)
    foreign static getTouchY(index)
    
    foreign static setPadVibration(leftRumble, rightRumble)
    foreign static setPadLightbarColor(r, g, b)
    foreign static resetPadLightbarColor()
}
)";

BX_SCRIPT_API_REGISTRATION(Window)
{
	Script::Get().BeginModule("window");
	{
		Script::Get().BeginEnumClass<GamepadButton>("GamepadButton");
		{
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::BUTTON_SOUTH>("south");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::BUTTON_EAST>("east");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::BUTTON_WEST>("west");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::BUTTON_NORTH>("north");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::SHOULDER_LEFT>("shoulderLeft");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::SHOULDER_RIGHT>("shoulderRight");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::BUTTON_SELECT>("select");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::BUTTON_START>("start");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::STICK_LEFT>("leftStickPress");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::STICK_RIGHT>("rightStickPress");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::DPAD_UP>("dPadUp");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::DPAD_RIGHT>("dPadRight");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::DPAD_DOWN>("dPadDown");
			Script::Get().BindEnumVal<GamepadButton, GamepadButton::DPAD_LEFT>("dPadLeft");
		}
		Script::Get().EndClass();

		Script::Get().BeginEnumClass<GamepadAxis>("GamepadAxis");
		{
			Script::Get().BindEnumVal<GamepadAxis, GamepadAxis::STICK_LEFT_X>("leftStickX");
			Script::Get().BindEnumVal<GamepadAxis, GamepadAxis::STICK_LEFT_Y>("leftStickY");
			Script::Get().BindEnumVal<GamepadAxis, GamepadAxis::STICK_RIGHT_X>("rightStickX");
			Script::Get().BindEnumVal<GamepadAxis, GamepadAxis::STICK_RIGHT_Y>("rightStickY");
			Script::Get().BindEnumVal<GamepadAxis, GamepadAxis::TRIGGER_LEFT>("leftTrigger");
			Script::Get().BindEnumVal<GamepadAxis, GamepadAxis::TRIGGER_RIGHT>("rightTrigger");
		}
		Script::Get().EndClass();

		Script::Get().BeginEnumClass<MouseButton>("MouseButton");
		{
			Script::Get().BindEnumVal<MouseButton, MouseButton::MOUSE_BUTTON_LEFT>("left");
			Script::Get().BindEnumVal<MouseButton, MouseButton::MOUSE_BUTTON_RIGHT>("right");
			Script::Get().BindEnumVal<MouseButton, MouseButton::MOUSE_BUTTON_MIDDLE>("middle");
		}
		Script::Get().EndClass();

		Script::Get().BeginEnumClass<Key>("Key");
		{
			Script::Get().BindEnumVal<Key, Key::RIGHT>("right");
			Script::Get().BindEnumVal<Key, Key::LEFT>("left");
			Script::Get().BindEnumVal<Key, Key::DOWN>("down");
			Script::Get().BindEnumVal<Key, Key::UP>("up");
			Script::Get().BindEnumVal<Key, Key::SPACE>("space");
			Script::Get().BindEnumVal<Key, Key::ESCAPE>("escape");
			Script::Get().BindEnumVal<Key, Key::ENTER>("enter");

			Script::Get().BindEnumVal<Key, Key::A>("a");
			Script::Get().BindEnumVal<Key, Key::B>("b");
			Script::Get().BindEnumVal<Key, Key::C>("c");
			Script::Get().BindEnumVal<Key, Key::D>("d");
			Script::Get().BindEnumVal<Key, Key::E>("e");
			Script::Get().BindEnumVal<Key, Key::F>("f");
			Script::Get().BindEnumVal<Key, Key::G>("g");
			Script::Get().BindEnumVal<Key, Key::H>("h");
			Script::Get().BindEnumVal<Key, Key::I>("i");
			Script::Get().BindEnumVal<Key, Key::J>("j");
			Script::Get().BindEnumVal<Key, Key::K>("k");
			Script::Get().BindEnumVal<Key, Key::L>("l");
			Script::Get().BindEnumVal<Key, Key::M>("m");
			Script::Get().BindEnumVal<Key, Key::N>("n");
			Script::Get().BindEnumVal<Key, Key::O>("o");
			Script::Get().BindEnumVal<Key, Key::P>("p");
			Script::Get().BindEnumVal<Key, Key::Q>("q");
			Script::Get().BindEnumVal<Key, Key::R>("r");
			Script::Get().BindEnumVal<Key, Key::S>("s");
			Script::Get().BindEnumVal<Key, Key::T>("t");
			Script::Get().BindEnumVal<Key, Key::U>("u");
			Script::Get().BindEnumVal<Key, Key::V>("v");
			Script::Get().BindEnumVal<Key, Key::W>("w");
			Script::Get().BindEnumVal<Key, Key::X>("x");
			Script::Get().BindEnumVal<Key, Key::Y>("y");
			Script::Get().BindEnumVal<Key, Key::Z>("z");
		}
		Script::Get().EndClass();

		Script::Get().BeginClass("Window");
		{
			Script::Get().BindFunction<decltype(&Window::GetAxis), &Window::GetAxis>(true, "getAxis(_)");
			Script::Get().BindFunction<decltype(&Window::GetButton), &Window::GetButton>(true, "getButton(_)");
			Script::Get().BindFunction<decltype(&Window::GetButtonOnce), &Window::GetButtonOnce>(true, "getButtonOnce(_)");

			Script::Get().BindFunction<decltype(&Window::GetKey), &Window::GetKey>(true, "getKey(_)");
			Script::Get().BindFunction<decltype(&Window::GetKeyOnce), &Window::GetKeyOnce>(true, "getKeyOnce(_)");

			//Script::Get().BindFunction<decltype(&Window::GetMouse), &Window::GetMouse>(true, "getMouse()");
			Script::Get().BindFunction<decltype(&Window::GetMouseButton), &Window::GetMouseButton>(true, "getMouseButton(_)");
			Script::Get().BindFunction<decltype(&Window::GetMouseButtonOnce), &Window::GetMouseButtonOnce>(true, "getMouseButtonOnce(_)");
			Script::Get().BindFunction<decltype(&Window::GetMouseX), &Window::GetMouseX>(true, "getMouseX()");
			Script::Get().BindFunction<decltype(&Window::GetMouseY), &Window::GetMouseY>(true, "getMouseY()");

			Script::Get().BindFunction<decltype(&Window::GetNumTouches), &Window::GetNumTouches>(true, "getNumTouches()");
			Script::Get().BindFunction<decltype(&Window::GetTouchId), &Window::GetTouchId>(true, "getTouchId(_)");
			Script::Get().BindFunction<decltype(&Window::GetTouchX), &Window::GetTouchX>(true, "getTouchX(_)");
			Script::Get().BindFunction<decltype(&Window::GetTouchY), &Window::GetTouchY>(true, "getTouchY(_)");

			Script::Get().BindFunction<decltype(&Window::SetPadVibration), &Window::SetPadVibration>(true, "setPadVibration(_,_)");
			Script::Get().BindFunction<decltype(&Window::SetPadLightbarColor), &Window::SetPadLightbarColor>(true, "setPadLightbarColor(_,_,_)");
			Script::Get().BindFunction<decltype(&Window::ResetPadLightbarColor), &Window::ResetPadLightbarColor>(true, "resetPadLightbarColor()");
		}
		Script::Get().EndClass();
	}
	Script::Get().EndModule();

	ScriptModuleSource src{};
	src.moduleName = "window";
	src.moduleSource = g_windowSrc;
	return src;
}