#include "bxl.hpp"

#include <iostream>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>

// -----------------------------------------------------------------------------
// App timing & input
// -----------------------------------------------------------------------------
namespace bx
{
	static GLFWwindow* g_window = nullptr;

	static f64 g_last_time  = 0.0;
	static f64 g_delta_time = 0.0;

	static bool g_key_down[GLFW_KEY_LAST + 1]     = {};
	static bool g_key_pressed[GLFW_KEY_LAST + 1]  = {};
	static bool g_key_released[GLFW_KEY_LAST + 1] = {};

	static bool g_mouse_down[8] = {};
	static f64 g_mouse_x        = 0.0, g_mouse_y = 0.0;
}

#if defined(BX_GFX_OPENGL) || defined(BX_GFX_OPENGLES)
#include <glad/glad.h>
#include <imgui_impl_opengl3.h>

static bool gl_init(const bx::app_config_t& config);
static void gl_shutdown();
static void gl_begin_frame();
static void gl_end_frame();
#endif

#ifdef BX_GFX_VULKAN
#include <imgui_impl_vulkan.h>

static bool vk_init(const bx::app_config_t& config);
static void vk_shutdown();
static void vk_begin_frame();
static void vk_end_frame();
#endif

static void glfw_error_callback(const i32 error, cstring desc)
{
	bx_loge(fmt::format("GLFW Error {}: {}", error, desc).c_str());
}

static void glfw_key_callback(GLFWwindow*, const i32 key, i32 scancode, const i32 action, i32 mods)
{
	if (key < 0 || key > GLFW_KEY_LAST)
		return;

	if (action == GLFW_PRESS)
	{
		bx::g_key_down[key]    = true;
		bx::g_key_pressed[key] = true;
	}
	else if (action == GLFW_RELEASE)
	{
		bx::g_key_down[key]     = false;
		bx::g_key_released[key] = true;
	}
}

static void glfw_mouse_button_callback(GLFWwindow*, const i32 button, const i32 action, i32 mods)
{
	if (button < 0 || button >= 8)
		return;
	bx::g_mouse_down[button] = (action == GLFW_PRESS);
}

static void glfw_cursor_pos_callback(GLFWwindow*, const f64 xpos, const f64 ypos)
{
	bx::g_mouse_x = xpos;
	bx::g_mouse_y = ypos;
}

bx::result_t bx::app_init(const app_config_t& config) noexcept
{
#ifdef __arm__
	if (putenv((char*)"DISPLAY=:0"))
	{
		bx_loge("Failed to set DISPLAY enviroment variable");
		return result_t::FAIL;
	}
#endif

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		bx_loge("Failed to init GLFW");
		return result_t::FAIL;
	}

#ifdef BX_GFX_VULKAN
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

#ifdef BX_GFX_OPENGL
	// Request context with compute shader support
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
#endif

#ifdef BX_GFX_OPENGLES
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_SAMPLES, 2);
#endif

	//pMonitor = glfwGetPrimaryMonitor();
	//const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	//width = pMode->width;
	//height = pMode->height;

	//glfwWindowHint(GLFW_RED_BITS, pMode->redBits);
	//glfwWindowHint(GLFW_GREEN_BITS, pMode->greenBits);
	//glfwWindowHint(GLFW_BLUE_BITS, pMode->blueBits);
	//glfwWindowHint(GLFW_REFRESH_RATE, pMode->refreshRate);

	const f32 main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
	const i32 scaled_width = static_cast<i32>(main_scale * static_cast<f32>(config.width));
	const i32 scaled_height = static_cast<i32>(main_scale * static_cast<f32>(config.height));
	g_window = glfwCreateWindow(scaled_width, scaled_height, config.title, nullptr, nullptr);
	if (!g_window)
	{
		bx_loge("Failed to create GLFW window");
		glfwTerminate();
		return result_t::FAIL;
	}

	glfwSetKeyCallback(g_window, glfw_key_callback);
	glfwSetMouseButtonCallback(g_window, glfw_mouse_button_callback);
	glfwSetCursorPosCallback(g_window, glfw_cursor_pos_callback);
	g_last_time = glfwGetTime();

#if defined(BX_GFX_OPENGL) || defined(BX_GFX_OPENGLES)
	if (!gl_init(config))
	{
		bx_loge("Failed to initialize GLFW graphics.");
		return result_t::FAIL;
	}
#endif

#ifdef BX_GFX_VULKAN
	if (!vk_init(config))
	{
		bx_loge("Failed to initialize GLFW graphics.");
		return result_t::FAIL;
	}
#endif

	return result_t::OK;
}

void bx::app_shutdown() noexcept
{
#ifdef BX_GFX_VULKAN
	vk_shutdown();
#endif

#if defined(BX_GFX_OPENGL) || defined(BX_GFX_OPENGLES)
	gl_shutdown();
#endif

	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(g_window);
	glfwTerminate();
}

bool bx::app_begin_frame() noexcept
{
	if (glfwWindowShouldClose(g_window))
		return false;

	glfwPollEvents();

	// Called once per frame, typically at the start of app_begin_frame()
	const f64 current_time = glfwGetTime();
	g_delta_time = current_time - g_last_time;
	g_last_time = current_time;

	// Reset one-frame flags
	std::fill_n(g_key_pressed, GLFW_KEY_LAST + 1, false);
	std::fill_n(g_key_released, GLFW_KEY_LAST + 1, false);

	/*if (glfwGetWindowAttrib(g_window, GLFW_ICONIFIED) != 0)
	{
		ImGui_ImplGlfw_Sleep(10);
		continue;
	}*/

	ImGui_ImplGlfw_NewFrame();

#ifdef BX_GFX_VULKAN
	vk_begin_frame();
#endif

#if defined(BX_GFX_OPENGL) || defined(BX_GFX_OPENGLES)
	gl_begin_frame();
#endif

	ImGui::NewFrame();
	return true;
}

void bx::app_end_frame(const bool present, const bool should_close) noexcept
{
	ImGui::ShowDemoWindow(0);

	if (present)
	{
		ImGui::Render();

#ifdef BX_GFX_VULKAN
		vk_end_frame();
#endif

#if defined(BX_GFX_OPENGL) || defined(BX_GFX_OPENGLES)
		gl_end_frame();
#endif
	}

	const bool close = glfwWindowShouldClose(g_window) || should_close;
	glfwSetWindowShouldClose(g_window, close);
}

f64 bx::app_time_seconds() noexcept
{
	return glfwGetTime();
}

f64 bx::app_frame_time() noexcept
{
	return g_delta_time;
}

bool bx::app_key_down(const i32 key) noexcept
{
	if (key < 0 || key > GLFW_KEY_LAST)
		return false;
	return g_key_down[key];
}

bx::key_state_t bx::app_key(const i32 key) noexcept
{
	key_state_t state{};
	if (key >= 0 && key <= GLFW_KEY_LAST)
	{
		state.down     = g_key_down[key];
		state.pressed  = g_key_pressed[key];
		state.released = g_key_released[key];
	}
	return state;
}

bx::mouse_state_t bx::app_mouse() noexcept
{
	mouse_state_t s{};
	s.x = static_cast<f32>(g_mouse_x);
	s.y = static_cast<f32>(g_mouse_y);
	for (i32 i       = 0; i < 8; ++i)
		s.buttons[i] = g_mouse_down[i];
	return s;
}

void bx::app_set_cursor_visible(const bool visible) noexcept
{
	glfwSetInputMode(g_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

// Backends

#if defined(BX_GFX_OPENGL) || defined(BX_GFX_OPENGLES)
static bool gl_init(const bx::app_config_t& config)
{
	glfwMakeContextCurrent(bx::g_window);
	glfwSwapInterval(config.vsync ? GLFW_TRUE : GLFW_FALSE);

	// Load GL function pointers
#ifdef BX_GFX_OPENGL
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
#else
	if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
#endif
	{
		bx_loge("Failed to load GLAD");
		return false;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	const f32 main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3
	io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
	io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.
#endif

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Print GPU/GL info
	bx_logv(fmt::format("GL version: {}", (cstring)glGetString(GL_VERSION)).c_str());
	bx_logv(fmt::format("GLSL version: {}", (cstring)glGetString(GL_SHADING_LANGUAGE_VERSION)).c_str());

	// Setup Platform/Renderer backends
	if (!ImGui_ImplGlfw_InitForOpenGL(bx::g_window, true))
	{
		bx_loge("Failed to initialize ImGui GLFW backend!");
		return false;
	}

#ifdef BX_GFX_OPENGL
	if (!ImGui_ImplOpenGL3_Init("#version 460 core\n"))
#else
	if (!ImGui_ImplOpenGL3_Init("#version 300 es\n"))
#endif
	{
		bx_loge("Failed to initialize ImGui OpenGL backend!");
		return false;
	}

	return true;
}

static void gl_shutdown()
{
	ImGui_ImplOpenGL3_Shutdown();
}

static void gl_begin_frame()
{
	ImGui_ImplOpenGL3_NewFrame();
}

static void gl_end_frame()
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(bx::g_window);
}
#endif

#ifdef BX_GFX_VULKAN
namespace bx
{
	static VkAllocationCallbacks* g_allocator = nullptr;
	static VkInstance g_instance = VK_NULL_HANDLE;
	static VkSurfaceKHR g_surface = VK_NULL_HANDLE;

	static VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
	static VkDevice g_device = VK_NULL_HANDLE;
	static u32 g_queue_family = static_cast<u32>(-1);
	static VkQueue g_queue = VK_NULL_HANDLE;
	static VkPipelineCache g_pipeline_cache = VK_NULL_HANDLE;
	static VkDescriptorPool g_descriptor_pool = VK_NULL_HANDLE;

	static VkFormat g_swapchain_format = VK_FORMAT_UNDEFINED;

	static VkCommandBuffer g_imgui_cmd_buffer = VK_NULL_HANDLE;
	static VkPipeline g_imgui_pipeline = VK_NULL_HANDLE;
	static VkRenderPass g_imgui_render_pass = VK_NULL_HANDLE;

	static ImGui_ImplVulkanH_Window g_main_window_data{};
	static uint32_t g_min_image_count = 2;
	static bool g_swap_chain_rebuild = false;
}

static void vk_check_result(const VkResult err)
{
	if (err != VK_SUCCESS)
	{
		bx_loge(fmt::format("[Vulkan] Error: VkResult = {}", err).c_str());
		if (err < 0)
			abort();
	}
}

static bool vk_is_extension_available(const std::vector<VkExtensionProperties>& properties, const char* extension)
{
	for (const VkExtensionProperties& p : properties)
		if (strcmp(p.extensionName, extension) == 0)
			return true;
	return false;
}

static bool vk_init(const bx::app_config_t& config)
{
	if (!glfwVulkanSupported())
	{
		bx_loge("Failed to get Vulkan support");
		glfwTerminate();
		return result_t::FAIL;
	}

	std::vector<const char*> instance_extensions{};
	u32 glfw_extensions_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
	for (uint32_t i = 0; i < glfw_extensions_count; i++)
		instance_extensions.push_back(glfw_extensions[i]);

	VkResult err;
	// Create Vulkan Instance
	{
		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		// Enumerate available extensions
		uint32_t properties_count;
		vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
		auto properties = std::vector<VkExtensionProperties>(properties_count);
		err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data());
		vk_check_result(err);

		// Enable required extensions
		if (vk_is_extension_available(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
			instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
		if (vk_is_extension_available(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
		{
			instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
			create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		}
#endif

		// Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
		const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
		create_info.enabledLayerCount = 1;
		create_info.ppEnabledLayerNames = layers;
		instance_extensions.push_back("VK_EXT_debug_report");
#endif

		// Create Vulkan Instance
		create_info.enabledExtensionCount = instance_extensions.size();
		create_info.ppEnabledExtensionNames = instance_extensions.data();
		err = vkCreateInstance(&create_info, g_allocator, &g_instance);
		vk_check_result(err);

		// Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
		auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
			g_Instance, "vkCreateDebugReportCallbackEXT");
		IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
		VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
		debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debug_report_ci.pfnCallback = debug_report;
		debug_report_ci.pUserData = nullptr;
		err = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
		vk_check_result(err);
#endif
	}

	// Create Window Surface
	err = glfwCreateWindowSurface(g_instance, g_window, g_allocator, &g_surface);
	vk_check_result(err);

	// Select Physical Device (GPU)
	g_physical_device = ImGui_ImplVulkanH_SelectPhysicalDevice(g_instance);
	IM_ASSERT(g_physical_device != VK_NULL_HANDLE);

	// Select graphics queue family
	g_queue_family = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_physical_device);
	IM_ASSERT(g_queue_family != -1);

	// Create Logical Device (with 1 queue)
	{
		ImVector<const char*> device_extensions;
		device_extensions.push_back("VK_KHR_swapchain");

		// Enumerate physical device extension
		uint32_t properties_count;
		ImVector<VkExtensionProperties> properties;
		vkEnumerateDeviceExtensionProperties(g_physical_device, nullptr, &properties_count, nullptr);
		properties.resize(properties_count);
		vkEnumerateDeviceExtensionProperties(g_physical_device, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
		if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
			device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

		const float queue_priority[] = { 1.0f };
		VkDeviceQueueCreateInfo queue_info[1] = {};
		queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[0].queueFamilyIndex = g_queue_family;
		queue_info[0].queueCount = 1;
		queue_info[0].pQueuePriorities = queue_priority;
		VkDeviceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
		create_info.pQueueCreateInfos = queue_info;
		create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
		create_info.ppEnabledExtensionNames = device_extensions.Data;
		err = vkCreateDevice(g_physical_device, &create_info, g_allocator, &g_device);
		vk_check_result(err);
		vkGetDeviceQueue(g_device, g_queue_family, 0, &g_queue);
	}

	// Create Descriptor Pool
	// If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 0;
		for (VkDescriptorPoolSize& pool_size : pool_sizes)
			pool_info.maxSets += pool_size.descriptorCount;
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		err = vkCreateDescriptorPool(g_device, &pool_info, g_allocator, &g_descriptor_pool);
		vk_check_result(err);
	}

	// Create Framebuffers
	int w, h;
	glfwGetFramebufferSize(g_window, &w, &h);
	ImGui_ImplVulkanH_Window* wd = &g_main_window_data;
	wd->Surface = g_surface;

	// Check for WSI support
	VkBool32 res;
	vkGetPhysicalDeviceSurfaceSupportKHR(g_physical_device, g_queue_family, wd->Surface, &res);
	if (res != VK_TRUE)
	{
		fprintf(stderr, "Error no WSI support on physical device 0\n");
		exit(-1);
	}

	// Select Surface Format
	const VkFormat requestSurfaceImageFormat[] = {
		VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM
	};
	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
		g_physical_device, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
		requestSurfaceColorSpace);

	// Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
	VkPresentModeKHR present_modes[] = {
		VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR
	};
#else
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
	wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
		g_physical_device, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
	//printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

	// Create SwapChain, RenderPass, Framebuffer, etc.
	IM_ASSERT(g_min_image_count >= 2);
	ImGui_ImplVulkanH_CreateOrResizeWindow(
		g_instance, g_physical_device, g_device, wd, g_queue_family, g_allocator, w, h, g_min_image_count,
		0);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);
	// Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;
	// Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
	io.ConfigDpiScaleFonts = true;
	// [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
	io.ConfigDpiScaleViewports = true;
	// [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(g_window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	//init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
	init_info.Instance = g_instance;
	init_info.PhysicalDevice = g_physical_device;
	init_info.Device = g_device;
	init_info.QueueFamily = g_queue_family;
	init_info.Queue = g_queue;
	init_info.PipelineCache = g_pipeline_cache;
	init_info.DescriptorPool = g_descriptor_pool;
	init_info.MinImageCount = g_min_image_count;
	init_info.ImageCount = g_main_window_data.ImageCount;
	init_info.Allocator = g_allocator;
	init_info.PipelineInfoMain.RenderPass = g_main_window_data.RenderPass;
	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.CheckVkResultFn = vk_check_result;
	ImGui_ImplVulkan_Init(&init_info);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//style.FontSizeBase = 20.0f;
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
	//IM_ASSERT(font != nullptr);
}

static void vk_shutdown()
{
	VkResult err = vkDeviceWaitIdle(g_device);
	vk_check_result(err);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplVulkanH_DestroyWindow(g_instance, g_device, &g_main_window_data, g_allocator);

	vkDestroyDescriptorPool(g_device, g_descriptor_pool, g_allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
	// Remove the debug report callback
	auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
	f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

	vkDestroyDevice(g_device, g_allocator);
	vkDestroyInstance(g_instance, g_allocator);
}

static void vk_begin_frame()
{
	ImGui_ImplVulkan_NewFrame();

	// Resize swap chain?
	int fb_width, fb_height;
	glfwGetFramebufferSize(g_window, &fb_width, &fb_height);
	if (fb_width > 0 && fb_height > 0 && (g_swap_chain_rebuild || g_main_window_data.Width != fb_width || g_main_window_data.Height != fb_height))
	{
		ImGui_ImplVulkan_SetMinImageCount(g_min_image_count);
		ImGui_ImplVulkanH_CreateOrResizeWindow(g_instance, g_physical_device, g_device, &g_main_window_data, g_queue_family, g_allocator, fb_width, fb_height, g_min_image_count, 0);
		g_main_window_data.FrameIndex = 0;
		g_swap_chain_rebuild = false;
	}
}

static void vk_end_frame()
{
	ImDrawData* main_draw_data = ImGui::GetDrawData();
	const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	g_main_window_data.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
	g_main_window_data.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
	g_main_window_data.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
	g_main_window_data.ClearValue.color.float32[3] = clear_color.w;

	const auto draw_data = ImGui::GetDrawData();

	VkSemaphore image_acquired_semaphore = g_main_window_data.FrameSemaphores[g_main_window_data.SemaphoreIndex].ImageAcquiredSemaphore;
	VkSemaphore render_complete_semaphore = g_main_window_data.FrameSemaphores[g_main_window_data.SemaphoreIndex].RenderCompleteSemaphore;

	// Render frame
	if (!main_is_minimized)
	{
		VkResult err = vkAcquireNextImageKHR(g_device, g_main_window_data.Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &g_main_window_data.FrameIndex);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
			g_swap_chain_rebuild = true;
		if (err == VK_ERROR_OUT_OF_DATE_KHR)
			return;
		if (err != VK_SUBOPTIMAL_KHR)
			vk_check_result(err);

		const ImGui_ImplVulkanH_Frame* fd = &g_main_window_data.Frames[g_main_window_data.FrameIndex];
		{
			err = vkWaitForFences(g_device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
			vk_check_result(err);

			err = vkResetFences(g_device, 1, &fd->Fence);
			vk_check_result(err);
		}
		{
			err = vkResetCommandPool(g_device, fd->CommandPool, 0);
			vk_check_result(err);
			VkCommandBufferBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
			vk_check_result(err);
		}
		{
			VkRenderPassBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			info.renderPass = g_main_window_data.RenderPass;
			info.framebuffer = fd->Framebuffer;
			info.renderArea.extent.width = g_main_window_data.Width;
			info.renderArea.extent.height = g_main_window_data.Height;
			info.clearValueCount = 1;
			info.pClearValues = &g_main_window_data.ClearValue;
			vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
		}

		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

		// Submit command buffer
		vkCmdEndRenderPass(fd->CommandBuffer);
		{
			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			info.waitSemaphoreCount = 1;
			info.pWaitSemaphores = &image_acquired_semaphore;
			info.pWaitDstStageMask = &wait_stage;
			info.commandBufferCount = 1;
			info.pCommandBuffers = &fd->CommandBuffer;
			info.signalSemaphoreCount = 1;
			info.pSignalSemaphores = &render_complete_semaphore;

			err = vkEndCommandBuffer(fd->CommandBuffer);
			vk_check_result(err);
			err = vkQueueSubmit(g_queue, 1, &info, fd->Fence);
			vk_check_result(err);
		}
	}

	// Update and Render additional Platform Windows
	const ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	// Present Main Platform Window
	if (!main_is_minimized && !g_swap_chain_rebuild)
	{
		VkPresentInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &render_complete_semaphore;
		info.swapchainCount = 1;
		info.pSwapchains = &g_main_window_data.Swapchain;
		info.pImageIndices = &g_main_window_data.FrameIndex;
		const VkResult err = vkQueuePresentKHR(g_queue, &info);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
			g_swap_chain_rebuild = true;
		if (err == VK_ERROR_OUT_OF_DATE_KHR)
			return;
		if (err != VK_SUBOPTIMAL_KHR)
			vk_check_result(err);
		g_main_window_data.SemaphoreIndex = (g_main_window_data.SemaphoreIndex + 1) % g_main_window_data.SemaphoreCount; // Now we can use the next set of semaphores
	}
}
#endif