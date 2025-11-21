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
    
    set(BXL_DVC_BACKENDS "Null" "GLFW" "Custom")
    set(BXL_AUD_BACKENDS "Null" "PortAudio" "Custom")
    set(BXL_NET_BACKENDS "Null" "ENet" "Custom")
    set(BXL_GFX_BACKENDS "Null" "OpenGL" "OpenGLES" "Vulkan" "Direct11" "Direct12" "Metal" "WebGPU" "Custom")

    set(BXL_DVC_BACKEND "GLFW" CACHE STRING "bxl device backend")
    set_property(CACHE BXL_DVC_BACKEND PROPERTY STRINGS ${BXL_DVC_BACKENDS})

    set(BXL_AUD_BACKEND "PortAudio" CACHE STRING "bxl audio backend")
    set_property(CACHE BXL_AUD_BACKEND PROPERTY STRINGS ${BXL_AUD_BACKENDS})

    set(BXL_NET_BACKEND "ENet" CACHE STRING "bxl net backend")
    set_property(CACHE BXL_NET_BACKEND PROPERTY STRINGS ${BXL_NET_BACKENDS})

    set(BXL_GFX_BACKEND "OpenGL" CACHE STRING "bxl graphics backend")
    set_property(CACHE BXL_GFX_BACKEND PROPERTY STRINGS ${BXL_GFX_BACKENDS})
    
    add_subdirectory(extern)
    
    set(bxl_srcs
            "src/bxl_app.cpp"
    )

    set(bxl_libs bx fmt)

    set(bxl_defines)

    if(BXL_ENABLE_IMGUI)
        set(bxl_libs ${bxl_libs} imgui)
        set(bxl_defines ${bxl_defines} BXL_APP_IMGUI)
        message("Enabled bxl imgui support")
    endif()

    message("Using bxl device backend: ${BXL_DVC_BACKEND}")
    message("Using bxl audio backend: ${BXL_AUD_BACKEND}")
    message("Using bxl net backend: ${BXL_NET_BACKEND}")
    message("Using bxl graphics backend: ${BXL_GFX_BACKEND}")

    # Device backend
    if(BXL_DVC_BACKEND STREQUAL "Null")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_dvc_null.cpp"
        )
        set(bxl_defines ${bxl_defines} BXL_DVC_NULL)

    elseif(BXL_DVC_BACKEND STREQUAL "GLFW")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_dvc_glfw.cpp"
        )
        set(bxl_libs ${bxl_libs} glfw)
        set(bxl_defines ${bxl_defines} BXL_DVC_GLFW)

    elseif(BXL_DVC_BACKEND STREQUAL "Custom")
        message(WARNING "Using a custom bxl device backend. You must provide your own implementation!")
    else()
        message(FATAL_ERROR "Unsupported bxl device backend '${BXL_DVC_BACKEND}'! Please choose a supported backend.")
    endif()

    # Audio backend
    if(BXL_AUD_BACKEND STREQUAL "Null")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_aud_null.cpp"
        )
        set(bxl_defines ${bxl_defines} BXL_AUD_NULL)

    elseif(BXL_AUD_BACKEND STREQUAL "PortAudio")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_aud_pa.cpp"
        )
        set(bxl_libs ${bxl_libs})
        set(bxl_defines ${bxl_defines} BXL_AUD_PORTAUDIO)

    elseif(BXL_AUD_BACKEND STREQUAL "Custom")
        message(WARNING "Using a custom bxl audio backend. You must provide your own implementation!")
    else()
        message(FATAL_ERROR "Unsupported bxl audio backend '${BXL_AUD_BACKEND}'! Please choose a supported backend.")
    endif()

    # Net backend
    if(BXL_NET_BACKEND STREQUAL "Null")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_net_null.cpp"
        )
        set(bxl_defines ${bxl_defines} BXL_NET_NULL)

    elseif(BXL_NET_BACKEND STREQUAL "ENet")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_net_enet.cpp"
        )
        set(bxl_libs ${bxl_libs})
        set(bxl_defines ${bxl_defines} BXL_NET_ENET)

    elseif(BXL_NET_BACKEND STREQUAL "Custom")
        message(WARNING "Using a custom bxl net backend. You must provide your own implementation!")
    else()
        message(FATAL_ERROR "Unsupported bxl net backend '${BXL_NET_BACKEND}'! Please choose a supported backend.")
    endif()

    # Graphics backend
    if(BXL_GFX_BACKEND STREQUAL "Null")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_null.cpp"
        )
        set(bxl_defines ${bxl_defines} BXL_GFX_NULL)

    elseif(BXL_GFX_BACKEND STREQUAL "OpenGL")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_gl.cpp"
        )
        set(bxl_libs ${bxl_libs} glad)
        set(bxl_defines ${bxl_defines} BXL_GFX_OPENGL)

    elseif(BXL_GFX_BACKEND STREQUAL "OpenGLES")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_gl.cpp"
        )
        set(bxl_libs ${bxl_libs} glad)
        set(bxl_defines ${bxl_defines} BXL_GFX_OPENGLES)

    elseif(BXL_GFX_BACKEND STREQUAL "Vulkan")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_vk.cpp"
        )
        set(bxl_libs ${bxl_libs} vulkan)
        set(bxl_defines ${bxl_defines} BXL_GFX_VULKAN)

    elseif(BXL_GFX_BACKEND STREQUAL "Direct11")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_dx11.cpp"
        )
        set(bxl_libs ${bxl_libs})
        set(bxl_defines ${bxl_defines} BXL_GFX_DIRECT11)

    elseif(BXL_GFX_BACKEND STREQUAL "Direct12")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_dx12.cpp"
        )
        set(bxl_libs ${bxl_libs})
        set(bxl_defines ${bxl_defines} BXL_GFX_DIRECT12)

    elseif(BXL_GFX_BACKEND STREQUAL "Metal")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_mtl.cpp"
        )
        set(bxl_libs ${bxl_libs})
        set(bxl_defines ${bxl_defines} BXL_GFX_METAL)

    elseif(BXL_GFX_BACKEND STREQUAL "WebGPU")
        set(bxl_srcs ${bxl_srcs}
                "src/bxl_gfx_wgpu.cpp"
        )
        set(bxl_libs ${bxl_libs})
        set(bxl_defines ${bxl_defines} BXL_GFX_WEBGPU)

    elseif(BXL_GFX_BACKEND STREQUAL "Custom")
        message(WARNING "Using a custom bxl graphics backend. You must provide your own implementation!")
    else()
        message(FATAL_ERROR "Unsupported bxl graphics backend '${BXL_GFX_BACKEND}'! Please choose a supported backend.")
    endif()

    add_library(bxl_internal ${bxl_srcs})

    target_include_directories(bxl_internal PUBLIC "include/bxl" "src")
    target_link_libraries(bxl_internal PUBLIC ${bxl_libs})
    target_compile_definitions(bxl_internal PUBLIC ${bxl_defines})

    message("Using bxl libs: ${bxl_libs}")
    message("Using bxl defines: ${bxl_defines}")

    add_library(bxl STATIC "src/bxl.cpp")
    target_include_directories(bxl PUBLIC "include/bxl")
    target_link_libraries(bxl PUBLIC bx fmt)
    if(BXL_ENABLE_IMGUI)
        target_link_libraries(bxl PUBLIC imgui)
        target_compile_definitions(bxl PUBLIC BXL_APP_IMGUI)
    endif()
    target_link_libraries(bxl PRIVATE bxl_internal)
endif()