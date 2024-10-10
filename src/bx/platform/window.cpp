#include "bx/platform/window.hpp"
#include <bx/core/macros.hpp>

#include <rttr/type>
#include <rttr/registration>
#include <stdexcept>
#include <iostream>

Window& Window::Get()
{
    static std::shared_ptr<Window> instance;
    if (!instance)
    {
        const auto& derived = rttr::type::get<Window>().get_derived_classes();
        if (derived.size() == 0)
            throw std::runtime_error("No derived Window class found.");
    
        const auto& type = *derived.begin();
        rttr::variant var = type.create();
        instance = var.get_value<std::shared_ptr<Window>>();
    }
    return *instance;
}

RTTR_PLUGIN_REGISTRATION
{
    rttr::registration::class_<Window>("Window");
        //.method("Initialize", &Window::Initialize)
        //.method("Reload", &Window::Reload)
        //.method("Shutdown", &Window::Shutdown)
        //.method("IsOpen", &Window::IsOpen)
        //.method("PollEvents", &Window::PollEvents)
        //.method("Display", &Window::Display)
        //.method("GetSize", &Window::GetSize)
        //.method("SetCursorMode", &Window::SetCursorMode)
        //.method("GetProcAddress", &Window::GetProcAddress);
}