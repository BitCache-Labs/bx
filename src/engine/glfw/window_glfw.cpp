#include <engine/glfw/window_glfw.hpp>
#include <engine/guard.hpp>

#ifdef EDITOR_BUILD
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#endif

BX_TYPE_REGISTRATION
{
	rttr::registration
		::class_<WindowGLFW>("WindowGLFW");
		//.constructor();
		//.method("GetWindowPtr", &WindowGLFW::GetWindowPtr);
}

BX_MODULE_DEFINE(WindowGLFW)
BX_MODULE_DEFINE_INTERFACE(Window, WindowGLFW)

bool WindowGLFW::IsOpen()
{
	return !glfwWindowShouldClose(m_pWindow);
}

void WindowGLFW::GetSize(i32* width, i32* height)
{
	glfwGetFramebufferSize(m_pWindow, width, height);
}

void WindowGLFW::GetContentScale(f32* xscale, f32* yscale)
{
	glfwGetWindowContentScale(m_pWindow, xscale, yscale);
}

void WindowGLFW::SetCursorMode(CursorMode mode)
{
	switch (mode)
	{
	case CursorMode::NORMAL:
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		break;
	case CursorMode::HIDDEN:
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		break;
	case CursorMode::DISABLED:
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		break;
	case CursorMode::CAPTURED:
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
		break;
	default:
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

WindowGLProc WindowGLFW::GetProcAddress(const char* name)
{
	return glfwGetProcAddress(name);
}

#ifdef EDITOR_BUILD
bool WindowGLFW::InitializeImGui()
{
#if defined(GRAPHICS_OPENGL_BACKEND) || defined(GRAPHICS_OPENGLES_BACKEND)
	if (!ImGui_ImplGlfw_InitForOpenGL(m_pWindow, true))
#endif
	{
		BX_LOGE(Window, "Failed to initialize ImGui GLFW backend!");
		return false;
	}
	return true;
}

void WindowGLFW::ShutdownImGui()
{
	ImGui_ImplGlfw_Shutdown();
}

void WindowGLFW::NewFrameImGui()
{
	ImGui_ImplGlfw_NewFrame();
}

void WindowGLFW::EndFrameImGui()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}
#endif

void WindowGLFW::glfw_error_callback(i32 i, const char* c)
{
	BX_LOGE(Window, "GLFW ({}) {}", i, c);
}

void WindowGLFW::glfw_window_size_callback(GLFWwindow* window, i32 width, i32 height)
{
	auto& ctx = static_cast<WindowGLFW&>(Window::Get());
}

void WindowGLFW::glfw_joystick_callback(i32 joy, i32 event)
{
	auto& ctx = static_cast<WindowGLFW&>(Window::Get());
}

void WindowGLFW::glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	auto& ctx = static_cast<WindowGLFW&>(Window::Get());
}

void WindowGLFW::glfw_key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
	auto& ctx = static_cast<WindowGLFW&>(Window::Get());
}

void WindowGLFW::glfw_mousebutton_callback(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
	auto& ctx = static_cast<WindowGLFW&>(Window::Get());
}

bool WindowGLFW::Initialize()
{
	BX_LOGD(Window, "Window initializing ...");

#ifdef __arm__
	if (putenv((char*)"DISPLAY=:0"))
	{
		LOGE(Window, "Failed to set DISPLAY enviroment variable!");
		return false;
	}
#endif

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		BX_LOGE(Window, "Failed to initialize GLFW!");
		return false;
	}

	const String& title = "Titlte";// Data::GetString("Title", "Title", DataTarget::SYSTEM);
	i32 width = 800;// Data::GetInt("Width", 800, DataTarget::SYSTEM);
	i32 height = 600;// Data::GetInt("Height", 600, DataTarget::SYSTEM);

	GLFWmonitor* pMonitor = nullptr;

#if defined(GRAPHICS_OPENGL_BACKEND) || defined(GRAPHICS_OPENGLES_BACKEND)

#if defined DEBUG_BUILD || defined EDITOR_BUILD
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif

#ifdef GRAPHICS_OPENGL_BACKEND
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);

	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

#elif defined GRAPHICS_OPENGLES_BACKEND
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_SAMPLES, 2);

	pMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	width = pMode->width;
	height = pMode->height;

	glfwWindowHint(GLFW_RED_BITS, pMode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, pMode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, pMode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, pMode->refreshRate);
#endif

#else
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
#endif

	m_pWindow = glfwCreateWindow(width, height, title.c_str(), pMonitor, NULL);

	if (m_pWindow == NULL)
	{
		BX_LOGE(Window, "Failed to create GLFW window!");
		glfwTerminate();
		return false;
	}

	glfwGetWindowSize(m_pWindow, &width, &height);
	//Screen::SetWidth(width);
	//Screen::SetHeight(height);

	glfwSetWindowSizeCallback(m_pWindow, glfw_window_size_callback);

#if defined(GRAPHICS_OPENGL_BACKEND) || defined(GRAPHICS_OPENGLES_BACKEND)
	glfwMakeContextCurrent(m_pWindow);
	glfwSwapInterval(1);

#elif defined(GRAPHICS_VULKAN_BACKEND)
	if (!glfwVulkanSupported())
	{
		LOGE(Window, "GLFW: Vulkan Not Supported!");
		return false;
	}
#endif

	glfwSetJoystickCallback(glfw_joystick_callback);
	glfwSetCursorPosCallback(m_pWindow, glfw_cursor_position_callback);
	glfwSetKeyCallback(m_pWindow, glfw_key_callback);
	glfwSetMouseButtonCallback(m_pWindow, glfw_mousebutton_callback);

	BX_LOGD(Window, "Window initialized successfully.");
	return true;
}

void WindowGLFW::Shutdown()
{
	BX_LOGD(Window, "Window shutting down ...");

	glfwSetJoystickCallback(NULL);
	glfwSetCursorPosCallback(m_pWindow, NULL);
	glfwSetKeyCallback(m_pWindow, NULL);
	glfwSetMouseButtonCallback(m_pWindow, NULL);

	glfwDestroyWindow(m_pWindow);
	glfwTerminate();

	BX_LOGD(Window, "Window shutdown complete.");
}

void WindowGLFW::PollEvents()
{
	// Poll GLFW events
	glfwPollEvents();

	// Handle the ESC key to close the window
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(m_pWindow, true);
}

void WindowGLFW::Display()
{
	//PROFILE_FUNCTION();

#if defined(GRAPHICS_OPENGL_BACKEND) || defined(GRAPHICS_OPENGLES_BACKEND)
	glfwSwapBuffers(m_pWindow);
#endif
}

f32 WindowGLFW::GetAxis(GamepadAxis axis)
{
	BX_FAIL("TODO: Implement");
	return 0;
}

bool WindowGLFW::GetButton(GamepadButton button)
{
	BX_FAIL("TODO: Implement");
	return false;
}

bool WindowGLFW::GetButtonOnce(GamepadButton button)
{
	BX_FAIL("TODO: Implement");
	return false;
}

bool WindowGLFW::GetKey(Key key)
{
	BX_ENSURE(key >= GLFW_KEY_SPACE && key <= GLFW_KEY_LAST);
	return glfwGetKey(m_pWindow, Enum::as_value(key)) == GLFW_PRESS;
}

bool WindowGLFW::GetKeyOnce(Key key)
{
	BX_FAIL("TODO: Implement");
	return false;
}

bool WindowGLFW::GetMouseButton(MouseButton button)
{
	return glfwGetMouseButton(m_pWindow, Enum::as_value(button)) == GLFW_PRESS;
}

bool WindowGLFW::GetMouseButtonOnce(MouseButton button)
{
	BX_FAIL("TODO: Implement");
	return false;
}

f32 WindowGLFW::GetMouseX()
{
	f64 x, y;
	glfwGetCursorPos(m_pWindow, &x, &y);
	return (f32)x;
}

f32 WindowGLFW::GetMouseY()
{
	f64 x, y;
	glfwGetCursorPos(m_pWindow, &x, &y);
	return (f32)y;
}

i32 WindowGLFW::GetNumTouches()
{
	BX_FAIL("TODO: Implement");
	return 0;
}

i32 WindowGLFW::GetTouchId(i32 index)
{
	BX_FAIL("TODO: Implement");
	return 0;
}

f32 WindowGLFW::GetTouchX(i32 index)
{
	BX_FAIL("TODO: Implement");
	return 0;
}

f32 WindowGLFW::GetTouchY(i32 index)
{
	BX_FAIL("TODO: Implement");
	return 0;
}

void WindowGLFW::SetPadVibration(i32 leftRumble, i32 rightRumble)
{
	// Unimplemented on the PC?
	BX_FAIL("TODO: Implement");
}

void WindowGLFW::SetPadLightbarColor(f32 r, f32 g, f32 b)
{
	// Unimplemented on the PC? (specific dualshock 5 controller mechanic)
	BX_FAIL("TODO: Implement");
}

void WindowGLFW::ResetPadLightbarColor()
{
	// Unimplemented on the PC? (specific dualshock 5 controller mechanic)
	BX_FAIL("TODO: Implement");
}

GLFWwindow* WindowGLFW::GetWindowPtr() const
{
	return m_pWindow;
}