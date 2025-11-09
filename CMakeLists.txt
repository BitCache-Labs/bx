cmake_minimum_required(VERSION 3.10)
project(bx)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BX_LITE_BUILD "Build bx lite implementation" ON)

add_library(bx INTERFACE)

# Include directories
target_include_directories(bx INTERFACE include)

# Optional bx lite simple but complete implementation
if (BX_LITE_BUILD)
    option(BXL_USE_VULKAN "Build with Vulkan backend" OFF)
    
    # Features
    set(BX_VULKAN_SUPPORTED ${BX_USE_VULKAN}) # TODO: This should check if vulkan is supported
    
    add_subdirectory(extern)
    add_subdirectory(src)
endif()