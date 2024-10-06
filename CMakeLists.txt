cmake_minimum_required (VERSION 3.22)
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Compiler Version: ${CMAKE_C_COMPILER_VERSION}")

project (bx)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

option (BX_BUILD_EDITOR "Build as editor binaries" OFF)
option (BX_INSTALL "Install binaries" OFF)
#option (BUILD_TESTS "Build the test binaries" ON)

# Define options for window backend
set (BX_WINDOW_BACKEND "GLFW" CACHE STRING "Choose the window backend: GLFW")
set_property (CACHE BX_WINDOW_BACKEND PROPERTY STRINGS "GLFW")
message ("BX window backend: ${BX_WINDOW_BACKEND}")

if (BX_WINDOW_BACKEND STREQUAL "GLFW")
    message(STATUS "Using GLFW as the window backend")
    add_compile_definitions (BX_WINDOW_GLFW_BACKEND)

else ()
    message(FATAL_ERROR "Unknown window backend: ${BX_WINDOW_BACKEND}")
endif ()

# Define options for graphics backend
set (BX_GRAPHICS_BACKEND "OpenGL" CACHE STRING "Choose the graphics backend: OpenGL, OpenGLES, Vulkan")
set_property (CACHE BX_GRAPHICS_BACKEND PROPERTY STRINGS "OpenGL" "OpenGLES" "Vulkan")
message ("BX graphics backend: ${BX_GRAPHICS_BACKEND}")

if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
    message(STATUS "Using OpenGL as the graphics backend")
    add_compile_definitions (BX_GRAPHICS_OPENGL_BACKEND)

elseif (BX_GRAPHICS_BACKEND STREQUAL "OpenGLES")
    message(STATUS "Using OpenGLES as the graphics backend")
    add_compile_definitions (BX_GRAPHICS_OPENGLES_BACKEND)

elseif (BX_GRAPHICS_BACKEND STREQUAL "Vulkan")
    message(STATUS "Using Vulkan as the graphics backend")
    add_compile_definitions (BX_GRAPHICS_VULKAN_BACKEND)

else ()
    message(FATAL_ERROR "Unknown graphics backend: ${GRAPHICS_BACKEND}")
endif ()

# Define options for physics backend
set (BX_PHYSICS_BACKEND "Bullet" CACHE STRING "Choose the physics backend: Bullet")
set_property (CACHE BX_PHYSICS_BACKEND PROPERTY STRINGS "Bullet")
message ("BX physics backend: ${BX_PHYSICS_BACKEND}")

if (BX_PHYSICS_BACKEND STREQUAL "Bullet")
    message(STATUS "Using Bullet as the physics backend")
    add_compile_definitions (BX_PHYSICS_BULLET_BACKEND)

else ()
    message(FATAL_ERROR "Unknown physics backend: ${WINDOW_BACKEND}")
endif ()

# Define options for audio backend
set (BX_AUDIO_BACKEND "PortAudio" CACHE STRING "Choose the audio backend: PortAudio")
set_property (CACHE BX_AUDIO_BACKEND PROPERTY STRINGS "PortAudio")
message ("BX audio backend: ${BX_AUDIO_BACKEND}")

if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
    message(STATUS "Using PortAudio as the audio backend")
    add_compile_definitions (BX_AUDIO_PORTAUDIO_BACKEND)

else ()
    message(FATAL_ERROR "Unknown audio backend: ${WINDOW_BACKEND}")
endif ()

# TODO: Use Platform.hpp for the future
message ("CMake system name: ${CMAKE_SYSTEM_NAME}")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_compile_definitions (BX_PLATFORM_PC)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_compile_definitions (BX_PLATFORM_LINUX)
endif ()

message ("CMake build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions (BX_DEBUG_BUILD)
else ()
    add_compile_definitions (BX_RELEASE_BUILD)
endif ()
# ---------------------------------------

if (BX_INSTALL)
	set (PROJECT_PATH_DIR ".")
else ()
	set (PROJECT_PATH_DIR "${CMAKE_SOURCE_DIR}")
endif ()

add_compile_definitions (BX_PROJECT_PATH="${PROJECT_PATH_DIR}")
add_compile_definitions (BX_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

if (BX_BUILD_EDITOR)
    add_compile_definitions (BX_EDITOR_BUILD)
endif ()

add_subdirectory (extern)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_subdirectory (wren)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

# Setup common engine sources
set (BX_SRCS
	"src/bx/engine/core/byte_types.cpp"
	"src/bx/engine/core/data.cpp"
	"src/bx/engine/core/ecs.cpp"
	"src/bx/engine/core/file.cpp"
	"src/bx/engine/core/input.cpp"
	"src/bx/engine/core/log.cpp"
	"src/bx/engine/core/math.cpp"
	"src/bx/engine/core/memory.cpp"
	"src/bx/engine/core/module.cpp"
	"src/bx/engine/core/object.cpp"
	"src/bx/engine/core/profiler.cpp"
	"src/bx/engine/core/resource.cpp"
	"src/bx/engine/core/shared_library.cpp"
	"src/bx/engine/core/stream.cpp"
	"src/bx/engine/core/thread.cpp"
	"src/bx/engine/core/time.cpp"
	"src/bx/engine/core/uuid.cpp"

	"src/bx/engine/modules/audio.cpp"
	"src/bx/engine/modules/graphics.cpp"
	"src/bx/engine/modules/physics.cpp"
	"src/bx/engine/modules/script.cpp"
	"src/bx/engine/modules/window.cpp"
	"src/bx/engine/modules/imgui.cpp"

	"src/bx/engine/modules/graphics/type.cpp"
	"src/bx/engine/modules/graphics/type_validation.cpp"
	
	"src/bx/framework/components/animator.cpp"
	"src/bx/framework/components/attributes.cpp"
	"src/bx/framework/components/audio_listener.cpp"
	"src/bx/framework/components/audio_source.cpp"
	"src/bx/framework/components/camera.cpp"
	"src/bx/framework/components/character_controller.cpp"
	"src/bx/framework/components/collider.cpp"
	"src/bx/framework/components/light.cpp"
	"src/bx/framework/components/mesh_filter.cpp"
	"src/bx/framework/components/mesh_renderer.cpp"
	"src/bx/framework/components/rigidbody.cpp"
	"src/bx/framework/components/spline.cpp"
	"src/bx/framework/components/transform.cpp"
	"src/bx/framework/resources/animation.cpp"
	"src/bx/framework/resources/material.cpp"
	"src/bx/framework/resources/mesh.cpp"
	"src/bx/framework/resources/shader.cpp"
	"src/bx/framework/resources/skeleton.cpp"
	"src/bx/framework/resources/texture.cpp"
	"src/bx/framework/systems/renderer/blas_data_pool.cpp"
	"src/bx/framework/systems/renderer/fsr2_pass.cpp"
	"src/bx/framework/systems/renderer/gbuffer_pass.cpp"
	"src/bx/framework/systems/renderer/id_pass.cpp"
	"src/bx/framework/systems/renderer/material_pool.cpp"
	"src/bx/framework/systems/renderer/nert_pass.cpp"
	"src/bx/framework/systems/renderer/present_pass.cpp"
	"src/bx/framework/systems/renderer/reblur_pass.cpp"
	"src/bx/framework/systems/renderer/restir_di_pass.cpp"
	"src/bx/framework/systems/renderer/sky.cpp"
	"src/bx/framework/systems/renderer/taa_pass.cpp"
	"src/bx/framework/systems/renderer/wfpt_pass.cpp"
	"src/bx/framework/systems/renderer/write_indirect_args_pass.cpp"
	"src/bx/framework/systems/renderer.cpp"
	"src/bx/framework/systems/dynamics.cpp"
	"src/bx/framework/systems/acoustics.cpp"
	"src/bx/framework/gameobject.cpp"
)

set (BX_LIBS ctti wnaabi cereal fmt glm wren imgui file_embed)

# Setup window backend
if (BX_WINDOW_BACKEND STREQUAL "GLFW")
	set (BX_SRCS ${BX_SRCS}
		"src/bx/engine/modules/window/backend/window_glfw.cpp"
	)

    set (BX_LIBS ${BX_LIBS} glfw imgui_glfw)
endif()

# Setup graphics backend
if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
	set (BX_SRCS ${BX_SRCS}
		"src/bx/engine/modules/graphics/backend/opengl/conversion.cpp"
		"src/bx/engine/modules/graphics/backend/opengl/graphics_pipeline.cpp"
		"src/bx/engine/modules/graphics/backend/opengl/opengl_api.cpp"
		"src/bx/engine/modules/graphics/backend/opengl/shader.cpp"
		"src/bx/engine/modules/graphics/backend/opengl/validation.cpp"

		"src/bx/engine/modules/graphics/backend/graphics_opengl.cpp"
	)

    set (BX_LIBS ${BX_LIBS} glad imgui_opengl3)

elseif (BX_GRAPHICS_BACKEND STREQUAL "Vulkan")
	set (BX_SRCS ${BX_SRCS}
		"src/bx/engine/modules/graphics/backend/vulkan/acceleration_structure.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/buffer.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/cmd_list.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/cmd_queue.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/compute_pipeline.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/conversion.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/descriptor_pool.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/descriptor_set.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/descriptor_set_layout.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/device.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/extensions.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/fence.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/framebuffer.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/graphics_pipeline.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/image.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/instance.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/pfn.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/physical_device.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/render_pass.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/resource_state_tracker.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/sampler.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/semaphore.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/shader.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/spirv_compiler.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/swapchain.cpp"
		"src/bx/engine/modules/graphics/backend/vulkan/validation.cpp"

		"src/bx/engine/modules/graphics/backend/graphics_vulkan.cpp"
	)

    set (BX_LIBS ${BX_LIBS} vulkan glslang SPIRV imgui_vulkan)
endif ()

# FSR2
set (BX_LIBS ${BX_LIBS} fsr2)

# Setup physics backend
if (BX_PHYSICS_BACKEND STREQUAL "Bullet")
	set (BX_SRCS ${BX_SRCS}
		"src/bx/engine/modules/physics/backend/physics_bullet.cpp"
	)

    set (BX_LIBS ${BX_LIBS} bullet3) #box2d
endif ()

# Setup audio backend
if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
	set (BX_SRCS ${BX_SRCS}
		"src/bx/engine/modules/audio/backend/audio_portaudio.cpp"
	)

    set (BX_LIBS ${BX_LIBS} PortAudio)
endif ()

# Setup editor sources
if (BX_BUILD_EDITOR)
	set (BX_SRCS ${BX_SRCS}
		"src/bx/editor/core/asset_importer.cpp"
		"src/bx/editor/core/assets.cpp"
		"src/bx/editor/core/command.cpp"
		"src/bx/editor/core/selection.cpp"
		"src/bx/editor/core/toolbar.cpp"
		"src/bx/editor/core/view.cpp"

		"src/bx/editor/gizmos/spline_gizmo.cpp"
		"src/bx/editor/gizmos/transform_gizmo.cpp"

		"src/bx/editor/views/assets_view.cpp"
		"src/bx/editor/views/console_view.cpp"
		"src/bx/editor/views/data_view.cpp"
		"src/bx/editor/views/entity_view.cpp"
		"src/bx/editor/views/gameobject_view.cpp"
		"src/bx/editor/views/inspector_view.cpp"
		"src/bx/editor/views/profiler_view.cpp"
		"src/bx/editor/views/scene_view.cpp"
		"src/bx/editor/views/settings_view.cpp"
	)

	set (BX_LIBS ${BX_LIBS} stb assimp implot ImGuizmo IconFontCppHeaders FontAwesome FreeFonts)

	# Add editor runtime
	set (BX_SRCS ${BX_SRCS} "src/bx/runtime/editor_runtime.cpp")

else ()
	
	# Add game runtime
	set (BX_SRCS ${BX_SRCS} "src/bx/runtime/game_runtime.cpp")
endif ()

# Create bx lib
add_library (bx STATIC ${BX_SRCS})
target_link_libraries (bx ${BX_LIBS})
target_include_directories (bx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# TODO: clean this up
add_custom_command(TARGET bx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/fsr2/bin/ffx_fsr2_api_vk_x64.dll
		${CMAKE_BINARY_DIR}
    VERBATIM)

add_custom_command(TARGET bx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/fsr2/bin/ffx_fsr2_api_x64.dll
		${CMAKE_BINARY_DIR}
    VERBATIM)

if (MSVC)
	if (NOT BX_INSTALL)
		target_compile_options(bx PUBLIC "/Zi")
	endif ()

	target_link_options(bx PUBLIC "/INCREMENTAL")
endif ()

if (BX_INSTALL)
	add_compile_definitions (BX_INSTALL)

	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/extern/fsr2/bin/ffx_fsr2_api_vk_x64.dll ${CMAKE_CURRENT_SOURCE_DIR}/extern/fsr2/bin/ffx_fsr2_api_x64.dll DESTINATION bin)

	# Copy game contents to install dir
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/game DESTINATION bin)
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/extern/bx/include/bx/framework/assets/shaders DESTINATION bin/engine)

	# Package game in zip file of the installed files
	#add_custom_target(install_and_package
	#    COMMAND ${CMAKE_COMMAND} --install . --prefix ${CMAKE_INSTALL_PREFIX}
	#    COMMAND ${CMAKE_COMMAND} -E tar cfv "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.zip" --format=zip -- "${CMAKE_INSTALL_PREFIX}/."
	#    COMMENT "Creating distributable zip file"
	#)

	# Hide the console on installed builds
	#if (MSVC)
	#	target_link_options(bx INTERFACE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
	#endif ()
endif ()