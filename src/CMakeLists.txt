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
        bxl_app.cpp
)

if(BXL_USE_GLFW)
    set(bxl_srcs ${bxl_srcs}
            bxl_app_glfw.cpp
    )
endif()

if(BXL_USE_VULKAN)
    set(bxl_srcs ${bxl_srcs}
            bxl_gfx_vk.cpp
    )
else ()
    set(bxl_srcs ${bxl_srcs}
            #bxl_gfx_gl.cpp
            bxl_gfx_null.cpp
    )
endif()

add_library(bxl ${bxl_srcs})

target_include_directories(bxl PUBLIC include)
target_link_libraries(bxl PUBLIC bx fmt)

if(BXL_ENABLE_IMGUI)
    target_link_libraries(bxl PUBLIC imgui)
    target_compile_definitions(bxl PRIVATE BXL_APP_IMGUI)
endif()

if(BXL_USE_GLFW)
    target_link_libraries(bxl PRIVATE glfw)
    target_compile_definitions(bxl PRIVATE BXL_APP_GLFW)
endif()

if(BXL_USE_VULKAN)
    target_link_libraries(bxl PRIVATE vulkan)
    target_compile_definitions(bxl PRIVATE BXL_GFX_VULKAN)
else()
    target_link_libraries(bxl PRIVATE glad)
    target_compile_definitions(bxl PRIVATE BXL_GFX_OPENGL)
endif()