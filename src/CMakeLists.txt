# Source files
set(bxl_srcs
        bxl_app.cpp
        bxl_app_glfw.cpp
)

if(BX_USE_VULKAN AND BX_VULKAN_SUPPORTED)
    set(bxl_srcs ${bxl_srcs}
            bxl_gfx_vk.cpp
    )
else ()
    set(bxl_srcs ${bxl_srcs}
            bxl_gfx_gl.cpp
    )
endif()

add_library(bxl ${bxl_srcs})

# Include directories
target_include_directories(bxl PUBLIC include)

# Link libraries
target_link_libraries(bxl PUBLIC bx glfw glad imgui)

if(BX_VULKAN_SUPPORTED)
    target_link_libraries(bxl PUBLIC vulkan)
    target_compile_definitions(bxl PRIVATE BX_VULKAN_SUPPORTED)
endif()