if (BX_MATH)
    # GLM
    add_subdirectory(glm)

    # Klein
    #add_subdirectory(klein)
endif()

# FMT
add_subdirectory(fmt)

# GLFW
if(BX_APP_DVC_BACKEND STREQUAL "GLFW")
    add_subdirectory(glfw)
endif()

# Graphics
if(BX_APP_GFX_BACKEND STREQUAL "OpenGL" OR BX_APP_GFX_BACKEND STREQUAL "OpenGLES")
    # Glad
    set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile")
    set(GLAD_API "gl=4.6,gles2=3.2" CACHE STRING "API type/version pairs")
    set(GLAD_GENERATOR "c" CACHE STRING "Language to generate the binding for")
    set(GLAD_SPEC "gl" CACHE STRING "Name of the spec")
    set(GLAD_REPRODUCIBLE ON CACHE BOOL "Reproducible build")
    option(GLAD_ALL_EXTENSIONS "Include all extensions" ON) 
    option(GLAD_NO_LOADER "Generate loader" OFF) 

    add_subdirectory(glad)

    #add_library(glad glad/src/glad.c)
    #target_include_directories(glad PUBLIC glad/include)
    #target_link_libraries(glad ${OPENGL_LIBRARIES})

    # Link dl only on Linux/Unix
    #if(UNIX AND NOT APPLE)
    #    target_link_libraries(glad PUBLIC dl)
    #endif()
endif()

if(BX_APP_GFX_BACKEND STREQUAL "Vulkan")
    # TODO: Vulkan
endif()

# ImGui
if(BX_APP_ENABLE_IMGUI)
    set(imgui_srcs
            imgui/imgui.cpp
            imgui/imgui_demo.cpp
            imgui/imgui_draw.cpp
            imgui/imgui_tables.cpp
            imgui/imgui_widgets.cpp
    )

    set(imgui_libs)
    
    if(BX_APP_DVC_BACKEND STREQUAL "GLFW")
        set(imgui_srcs ${imgui_srcs}
                imgui/backends/imgui_impl_glfw.cpp
        )
        set(imgui_libs ${imgui_libs} glfw)

        if(UNIX AND NOT APPLE)
            set(imgui_libs ${imgui_libs} X11)
        endif()
    endif()

    if(BX_APP_GFX_BACKEND STREQUAL "OpenGL" OR BX_APP_GFX_BACKEND STREQUAL "OpenGLES")
        set(imgui_srcs ${imgui_srcs}
                imgui/backends/imgui_impl_opengl3.cpp
        )
    endif()

    if(BX_APP_GFX_BACKEND STREQUAL "Vulkan")
        set(imgui_srcs ${imgui_srcs}
                imgui/backends/imgui_impl_vulkan.cpp
        )
    endif()

    add_library(imgui ${imgui_srcs})
    target_include_directories(imgui PUBLIC imgui imgui/backends)
    target_link_libraries(imgui PRIVATE ${imgui_libs})
endif()

if (BX_TESTS)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(googletest)
endif()