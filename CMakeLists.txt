cmake_minimum_required (VERSION 3.22)
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Compiler Version: ${CMAKE_C_COMPILER_VERSION}")

project (bx)

set (CMAKE_CXX_STANDARD 11)
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
	"src/bx/framework/systems/renderer/id_pass.cpp"
	"src/bx/framework/systems/renderer/present_pass.cpp"
	"src/bx/framework/systems/renderer/srgb_to_linear_pass.cpp"
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
		"src/bx/engine/modules/graphics/backend/graphics_vulkan.cpp"
	)

    set (BX_LIBS ${BX_LIBS} vulkan glslang SPIRV imgui_vulkan)
endif ()

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

if (MSVC)
	target_compile_options(bx PUBLIC "/ZI")
	target_link_options(bx PUBLIC "/INCREMENTAL")
endif ()

if (BX_INSTALL)
	# Copy game contents to install dir
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/game DESTINATION bin)

	# Package game in zip file of the installed files
	#add_custom_target(install_and_package
	#    COMMAND ${CMAKE_COMMAND} --install . --prefix ${CMAKE_INSTALL_PREFIX}
	#    COMMAND ${CMAKE_COMMAND} -E tar cfv "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.zip" --format=zip -- "${CMAKE_INSTALL_PREFIX}/."
	#    COMMENT "Creating distributable zip file"
	#)

	# Hide the console on installed builds
	if (MSVC)
		target_link_options(bx INTERFACE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
	endif ()
endif ()