cmake_minimum_required(VERSION 3.16)
project(bx)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BX_LITE_BUILD "Build bx lite implementation" ON)

add_library(bx INTERFACE)
target_include_directories(bx INTERFACE "include/bx")

# Optional bx lite simple but complete implementation
if (BX_LITE_BUILD)
    option(BXL_ENABLE_IMGUI "Build with ImGui support" ON)
    option(BXL_USE_GLFW "Build with GLFW backend" ON)
    option(BXL_USE_VULKAN "Build with Vulkan backend" OFF)
    
    add_subdirectory(extern)
    
    if(BXL_ENABLE_IMGUI)
        message("Enabled ImGui support")
    endif()

    if(BXL_USE_GLFW)
        message("Using GLFW backend")
    endif()

    if(BXL_USE_VULKAN)
        message("Using Vulkan backend")
    else()
        message("Using OpenGL backend")
    endif()

    set(bxl_srcs
            "src/bxl_app.cpp"
    )

    if(BXL_USE_GLFW)
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_app_glfw.cpp"
        )
    endif()

    if(BXL_USE_VULKAN)
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_vk.cpp"
        )
    else ()
        set(bxl_srcs ${bxl_srcs}
                #"src/bxl_gfx_gl.cpp"
                "src/bxl_gfx_null.cpp"
        )
    endif()

    add_library(bxl_internal ${bxl_srcs})

    target_include_directories(bxl_internal PUBLIC "include/bxl" "src")
    target_link_libraries(bxl_internal PUBLIC bx fmt)

    if(BXL_ENABLE_IMGUI)
        target_link_libraries(bxl_internal PUBLIC imgui)
        target_compile_definitions(bxl_internal PUBLIC BXL_APP_IMGUI)
    endif()

    if(BXL_USE_GLFW)
        target_link_libraries(bxl_internal PUBLIC glfw)
        target_compile_definitions(bxl_internal PUBLIC BXL_APP_GLFW)
    endif()

    if(BXL_USE_VULKAN)
        target_link_libraries(bxl_internal PUBLIC vulkan)
        target_compile_definitions(bxl_internal PUBLIC BXL_GFX_VULKAN)
    else()
        target_link_libraries(bxl_internal PUBLIC glad)
        target_compile_definitions(bxl_internal PUBLIC BXL_GFX_OPENGL)
    endif()

    add_library(bxl STATIC "src/bxl.cpp")
    target_include_directories(bxl PUBLIC "include/bxl")
    target_link_libraries(bxl PUBLIC bx fmt)
    target_link_libraries(bxl PRIVATE bxl_internal)
endif()