# FMT
add_subdirectory(fmt)

# GLFW
add_subdirectory(glfw)

# Glad
add_library(glad glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include)
target_link_libraries(glad ${OPENGL_LIBRARIES})

# ImGui
set(imgui_srcs
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
)
if(BX_VULKAN_SUPPORTED)
    set(imgui_srcs ${imgui_srcs}
            imgui/backends/imgui_impl_vulkan.cpp
    )
endif()
add_library(imgui ${imgui_srcs})
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_link_libraries(imgui PRIVATE glfw)
if(UNIX AND NOT APPLE)
    target_link_libraries(imgui PRIVATE X11)
endif()