#include "bx/engine/modules/graphics/backend/vulkan/instance.hpp"

#include "bx/engine/core/macros.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/extensions.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/pfn.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/validation.hpp"

#ifdef BX_WINDOW_GLFW_BACKEND
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "bx/engine/modules/window/backend/window_glfw.hpp"
#endif

namespace Vk
{
    VkInstance CreateInstance(bool debug) {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "BX App";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "BX";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VULKAN_VERSION;

        std::vector<const char*> extensions = PlatformInstanceExtensions();
        for (size_t i = 0; i < instanceExtensions.size(); i++) {
            extensions.push_back(instanceExtensions[i]);
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (debug) {
            CheckValidationLayerSupport();
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        VkInstance instance;
        BX_ASSERT(!vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create instance.");
        return instance;
    }

    Instance::Instance(void* window, bool debug) {
        this->instance = CreateInstance(debug);
        this->surface = CreateSurface(window, this->instance);

        Pfn::Load(this->instance);

        if (debug) {
            VkDebugReportCallbackCreateInfoEXT createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            createInfo.pfnCallback = VulkanDebugCallback;
            createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;

            Pfn::vkCreateDebugReportCallbackEXT(this->instance, &createInfo, nullptr,
                &this->debugReportCallback);
        }
    }

    Instance::~Instance() {
        Pfn::vkDestroyDebugReportCallbackEXT(this->instance, this->debugReportCallback, nullptr);
        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
        vkDestroyInstance(this->instance, nullptr);
    }

    VkInstance Instance::GetInstance() const {
        return this->instance;
    }

    VkSurfaceKHR Instance::GetSurface() const {
        return this->surface;
    }

#ifdef BX_PLATFORM_PC
    VkSurfaceKHR Instance::CreateSurface(void* window, VkInstance instance) {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(reinterpret_cast<GLFWwindow*>(window));
        createInfo.hinstance = GetModuleHandle(nullptr);

        VkSurfaceKHR surface;
        BX_ASSERT(!vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface),
            "Failed to create surface.");
        return surface;
    }
#elif defined BX_PLATFORM_LINUX
    // TODO
#endif
}