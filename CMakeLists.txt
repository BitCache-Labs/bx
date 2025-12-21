cmake_minimum_required(VERSION 3.16)
project(bx)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BX_CORE "Build bx core lib" ON)
option(BX_STL "Build bx stl lib" ON)
option(BX_TYPE "Build bx type lib" ON)
option(BX_MATH "Build bx math lib" ON)
option(BX_ECS "Build bx ecs lib" OFF)
option(BX_APP "Build bx app lib" ON)
option(BX_ENGINE "Build bx engine lib" OFF)

option(BX_TESTS "Build bx tests" ON)
option(BX_EXAMPLES "Build bx examples" ON)

if (BX_CORE)
    add_library(bx_core STATIC "src/bx_core/bx_core.cpp")
    target_include_directories(bx_core PUBLIC "include/bx_core")
endif()

if (BX_STL)
    add_library(bx_stl INTERFACE)
    target_include_directories(bx_stl INTERFACE "include/bx_stl")
    target_link_libraries(bx_stl INTERFACE bx_core)
endif()

if (BX_TYPE)
    add_library(bx_type STATIC "src/bx_type/bx_type.cpp")
    target_include_directories(bx_type PUBLIC "include/bx_type")
    target_link_libraries(bx_type PUBLIC bx_core bx_stl)
endif()

if (BX_MATH)
    add_library(bx_math INTERFACE)
    target_include_directories(bx_math INTERFACE "include/bx_math")
    target_link_libraries(bx_math INTERFACE bx_core glm)
endif()

if (BX_APP)
    option(BX_APP_ENABLE_IMGUI "Build with ImGui support" ON)
    
    set(BX_APP_DVC_BACKENDS "Null" "GLFW" "Custom")
    set(BX_APP_AUD_BACKENDS "Null" "PortAudio" "Custom")
    set(BX_APP_NET_BACKENDS "Null" "ENet" "Custom")
    set(BX_APP_GFX_BACKENDS "Null" "OpenGL" "OpenGLES" "Vulkan" "Direct11" "Direct12" "Metal" "WebGPU" "Custom")

    set(BX_APP_DVC_BACKEND "GLFW" CACHE STRING "bx app device backend")
    set_property(CACHE BX_APP_DVC_BACKEND PROPERTY STRINGS ${BX_APP_DVC_BACKENDS})

    set(BX_APP_AUD_BACKEND "PortAudio" CACHE STRING "bx app audio backend")
    set_property(CACHE BX_APP_AUD_BACKEND PROPERTY STRINGS ${BX_APP_AUD_BACKENDS})

    set(BX_APP_NET_BACKEND "ENet" CACHE STRING "bx app net backend")
    set_property(CACHE BX_APP_NET_BACKEND PROPERTY STRINGS ${BX_APP_NET_BACKENDS})

    set(BX_APP_GFX_BACKEND "OpenGL" CACHE STRING "bx app graphics backend")
    set_property(CACHE BX_APP_GFX_BACKEND PROPERTY STRINGS ${BX_APP_GFX_BACKENDS})
    
    add_subdirectory(extern)
    
    set(bx_app_srcs
            "src/bx_app/bx_app.cpp"
    )

    set(bx_app_libs)

    set(bx_app_defines)

    if(BX_APP_ENABLE_IMGUI)
        set(bx_app_libs ${bx_app_libs} imgui)
        set(bx_app_defines ${bx_app_defines} BX_APP_APP_IMGUI)
        message("Enabled bx app imgui support")
    endif()

    message("Using bx app device backend: ${BX_APP_DVC_BACKEND}")
    message("Using bx app audio backend: ${BX_APP_AUD_BACKEND}")
    message("Using bx app net backend: ${BX_APP_NET_BACKEND}")
    message("Using bx app graphics backend: ${BX_APP_GFX_BACKEND}")

    # Device backend
    if(BX_APP_DVC_BACKEND STREQUAL "Null")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_dvc_null.cpp"
        )
        set(bx_app_defines ${bx_app_defines} BX_APP_DVC_NULL)

    elseif(BX_APP_DVC_BACKEND STREQUAL "GLFW")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_dvc_glfw.cpp"
        )
        set(bx_app_libs ${bx_app_libs} glfw)
        set(bx_app_defines ${bx_app_defines} BX_APP_DVC_GLFW)

    elseif(BX_APP_DVC_BACKEND STREQUAL "Custom")
        message(WARNING "Using a custom bx app device backend. You must provide your own implementation!")
    else()
        message(FATAL_ERROR "Unsupported bx app device backend '${BX_APP_DVC_BACKEND}'! Please choose a supported backend.")
    endif()

    # Audio backend
    if(BX_APP_AUD_BACKEND STREQUAL "Null")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_aud_null.cpp"
        )
        set(bx_app_defines ${bx_app_defines} BX_APP_AUD_NULL)

    elseif(BX_APP_AUD_BACKEND STREQUAL "PortAudio")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_aud_pa.cpp"
        )
        set(bx_app_libs ${bx_app_libs})
        set(bx_app_defines ${bx_app_defines} BX_APP_AUD_PORTAUDIO)

    elseif(BX_APP_AUD_BACKEND STREQUAL "Custom")
        message(WARNING "Using a custom bx app audio backend. You must provide your own implementation!")
    else()
        message(FATAL_ERROR "Unsupported bx app audio backend '${BX_APP_AUD_BACKEND}'! Please choose a supported backend.")
    endif()

    # Net backend
    if(BX_APP_NET_BACKEND STREQUAL "Null")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_net_null.cpp"
        )
        set(bx_app_defines ${bx_app_defines} BX_APP_NET_NULL)

    elseif(BX_APP_NET_BACKEND STREQUAL "ENet")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_net_enet.cpp"
        )
        set(bx_app_libs ${bx_app_libs})
        set(bx_app_defines ${bx_app_defines} BX_APP_NET_ENET)

    elseif(BX_APP_NET_BACKEND STREQUAL "Custom")
        message(WARNING "Using a custom bx app net backend. You must provide your own implementation!")
    else()
        message(FATAL_ERROR "Unsupported bx app net backend '${BX_APP_NET_BACKEND}'! Please choose a supported backend.")
    endif()

    # Graphics backend
    if(BX_APP_GFX_BACKEND STREQUAL "Null")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_gfx_null.cpp"
        )
        set(bx_app_defines ${bx_app_defines} BX_APP_GFX_NULL)

    elseif(BX_APP_GFX_BACKEND STREQUAL "OpenGL")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_gfx_gl.cpp"
        )
        set(bx_app_libs ${bx_app_libs} glad)
        set(bx_app_defines ${bx_app_defines} BX_APP_GFX_OPENGL)

    elseif(BX_APP_GFX_BACKEND STREQUAL "OpenGLES")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_gfx_gl.cpp"
        )
        set(bx_app_libs ${bx_app_libs} glad)
        set(bx_app_defines ${bx_app_defines} BX_APP_GFX_OPENGLES)

    elseif(BX_APP_GFX_BACKEND STREQUAL "Vulkan")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_gfx_vk.cpp"
        )
        set(bx_app_libs ${bx_app_libs} vulkan)
        set(bx_app_defines ${bx_app_defines} BX_APP_GFX_VULKAN)

    elseif(BX_APP_GFX_BACKEND STREQUAL "Direct11")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_gfx_dx11.cpp"
        )
        set(bx_app_libs ${bx_app_libs})
        set(bx_app_defines ${bx_app_defines} BX_APP_GFX_DIRECT11)

    elseif(BX_APP_GFX_BACKEND STREQUAL "Direct12")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_gfx_dx12.cpp"
        )
        set(bx_app_libs ${bx_app_libs})
        set(bx_app_defines ${bx_app_defines} BX_APP_GFX_DIRECT12)

    elseif(BX_APP_GFX_BACKEND STREQUAL "Metal")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_gfx_mtl.cpp"
        )
        set(bx_app_libs ${bx_app_libs})
        set(bx_app_defines ${bx_app_defines} BX_APP_GFX_METAL)

    elseif(BX_APP_GFX_BACKEND STREQUAL "WebGPU")
        set(bx_app_srcs ${bx_app_srcs}
                "src/bx_app/bx_gfx_wgpu.cpp"
        )
        set(bx_app_libs ${bx_app_libs})
        set(bx_app_defines ${bx_app_defines} BX_APP_GFX_WEBGPU)

    elseif(BX_APP_GFX_BACKEND STREQUAL "Custom")
        message(WARNING "Using a custom bx app graphics backend. You must provide your own implementation!")
    else()
        message(FATAL_ERROR "Unsupported bx app graphics backend '${BX_APP_GFX_BACKEND}'! Please choose a supported backend.")
    endif()

    add_library(bx_app_internal INTERFACE)

    target_include_directories(bx_app_internal INTERFACE "include/bx_app" "src/bx_app")
    target_link_libraries(bx_app_internal INTERFACE ${bx_app_libs})
    target_compile_definitions(bx_app_internal INTERFACE ${bx_app_defines})

    #message("Using bx app srcs: ${bx_app_srcs}")
    #message("Using bx app libs: ${bx_app_libs}")
    #message("Using bx app defines: ${bx_app_defines}")

    add_library(bx_app STATIC ${bx_app_srcs})
    target_include_directories(bx_app PUBLIC "include/bx_app")
    target_link_libraries(bx_app PUBLIC bx_core bx_stl bx_type fmt)
    if(BX_APP_ENABLE_IMGUI)
        target_link_libraries(bx_app PUBLIC imgui)
        target_compile_definitions(bx_app PUBLIC BX_APP_IMGUI)
    endif()
    target_link_libraries(bx_app PRIVATE bx_app_internal)
endif()

if (BX_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

if (BX_EXAMPLES)
    add_subdirectory(examples)
endif()