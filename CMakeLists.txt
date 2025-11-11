cmake_minimum_required(VERSION 3.16)
project(bx)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BX_LITE_BUILD "Build bx lite implementation" ON)

add_library(bx INTERFACE)
target_include_directories(bx INTERFACE include)

# Optional bx lite simple but complete implementation
if (BX_LITE_BUILD)
    option(BXL_ENABLE_IMGUI "Build with ImGui support" ON)
    option(BXL_USE_GLFW "Build with GLFW backend" ON)
    option(BXL_USE_VULKAN "Build with Vulkan backend" OFF)
    
    add_subdirectory(extern)
    add_subdirectory(src)
endif()