#include "bx/platform/window.hpp"
#include <bx/core/macros.hpp>
#include <bx/core/module.hpp>

#include <rttr/type>
#include <rttr/registration>
#include <stdexcept>
#include <iostream>

Window& Window::Get()
{
    return Module::Get<Window>();
}

RTTR_PLUGIN_REGISTRATION
{
    rttr::registration::class_<Window>("Window")
    .method("Initialize", &Window::Initialize)
    .method("Reload", &Window::Reload)
    .method("Shutdown", &Window::Shutdown)
    .method("IsOpen", &Window::IsOpen)
    .method("PollEvents", &Window::PollEvents)
    .method("Display", &Window::Display)
    .method("GetSize", &Window::GetSize)
    .method("SetCursorMode", &Window::SetCursorMode)
    .method("GetProcAddress", &Window::GetProcAddress)
    .method("InitializeImGui", &Window::InitializeImGui)
    .method("ShutdownImGui", &Window::ShutdownImGui)
    .method("NewFrameImGui", &Window::NewFrameImGui)
    .method("EndFrameImGui", &Window::EndFrameImGui);
}