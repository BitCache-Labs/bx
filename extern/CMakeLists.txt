# FMT
add_subdirectory(fmt)

# GLFW
if(BXL_USE_GLFW)
    add_subdirectory(glfw)
endif()

# Graphics
if(BXL_USE_VULKAN)
    # TODO: Vulkan
else()
    # Glad
    add_library(glad glad/src/glad.c)
    target_include_directories(glad PUBLIC glad/include)
    target_link_libraries(glad ${OPENGL_LIBRARIES})

    # Link dl only on Linux/Unix
    if(UNIX AND NOT APPLE)
        target_link_libraries(glad PUBLIC dl)
    endif()
endif()

# ImGui
if(BXL_ENABLE_IMGUI)
    set(imgui_srcs
            imgui/imgui.cpp
            imgui/imgui_demo.cpp
            imgui/imgui_draw.cpp
            imgui/imgui_tables.cpp
            imgui/imgui_widgets.cpp
    )
    
    if(BXL_USE_GLFW)
        set(imgui_srcs ${imgui_srcs}
                imgui/backends/imgui_impl_glfw.cpp
        )
    endif()

    if(BXL_USE_VULKAN)
        set(imgui_srcs ${imgui_srcs}
                imgui/backends/imgui_impl_vulkan.cpp
        )
    else()
        set(imgui_srcs ${imgui_srcs}
                imgui/backends/imgui_impl_opengl3.cpp
        )
    endif()

    add_library(imgui ${imgui_srcs})
    target_include_directories(imgui PUBLIC imgui imgui/backends)
    target_link_libraries(imgui PRIVATE glfw)

    if(BXL_USE_GLFW AND UNIX AND NOT APPLE)
        target_link_libraries(imgui PRIVATE X11)
    endif()
endif()