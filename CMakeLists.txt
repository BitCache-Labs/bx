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

# Define options for platform backend
set (BX_PLATFORM_BACKEND "Default" CACHE STRING "Choose the platform backend: Default")
set_property (CACHE BX_PLATFORM_BACKEND PROPERTY STRINGS "Default" "Custom")
message ("BX platform backend: ${BX_PLATFORM_BACKEND}")

if (BX_PLATFORM_BACKEND STREQUAL "Default")
    message(STATUS "Using the default platform backend")
    add_compile_definitions (BX_PLATFORM_DEFAULT_BACKEND)

elseif (BX_PLATFORM_BACKEND STREQUAL "Custom")
    message(STATUS "Using a custom platform backend")
    add_compile_definitions (BX_PLATFORM_CUSTOM_BACKEND)

else ()
    message(FATAL_ERROR "Unknown platform backend: ${BX_PLATFORM_BACKEND}")
endif ()

# Define options for window backend
set (BX_WINDOW_BACKEND "GLFW" CACHE STRING "Choose the window backend: GLFW")
set_property (CACHE BX_WINDOW_BACKEND PROPERTY STRINGS "GLFW" "Custom")
message ("BX window backend: ${BX_WINDOW_BACKEND}")

if (BX_WINDOW_BACKEND STREQUAL "GLFW")
    message(STATUS "Using GLFW as the window backend")
    add_compile_definitions (BX_WINDOW_GLFW_BACKEND)

elseif (BX_WINDOW_BACKEND STREQUAL "Custom")
    message(STATUS "Using a custom window backend")
    add_compile_definitions (BX_WINDOW_CUSTOM_BACKEND)

else ()
    message(FATAL_ERROR "Unknown window backend: ${BX_WINDOW_BACKEND}")
endif ()

# Define options for graphics backend
set (BX_GRAPHICS_BACKEND "OpenGL" CACHE STRING "Choose the graphics backend: OpenGL, OpenGLES, Vulkan")
set_property (CACHE BX_GRAPHICS_BACKEND PROPERTY STRINGS "OpenGL" "OpenGLES" "Vulkan" "Custom")
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

elseif (BX_GRAPHICS_BACKEND STREQUAL "Custom")
    message(STATUS "Using a custom graphics backend")
    add_compile_definitions (BX_GRAPHICS_CUSTOM_BACKEND)

else ()
    message(FATAL_ERROR "Unknown graphics backend: ${GRAPHICS_BACKEND}")
endif ()

# Define options for audio backend
set (BX_AUDIO_BACKEND "PortAudio" CACHE STRING "Choose the audio backend: PortAudio")
set_property (CACHE BX_AUDIO_BACKEND PROPERTY STRINGS "PortAudio" "Custom")
message ("BX audio backend: ${BX_AUDIO_BACKEND}")

if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
    message(STATUS "Using PortAudio as the audio backend")
    add_compile_definitions (BX_AUDIO_PORTAUDIO_BACKEND)

elseif (BX_AUDIO_BACKEND STREQUAL "Custom")
    message(STATUS "Using a custom audio backend")
    add_compile_definitions (BX_AUDIO_CUSTOM_BACKEND)

else ()
    message(FATAL_ERROR "Unknown audio backend: ${WINDOW_BACKEND}")
endif ()



# External libraries
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



# Setup bx stack library
add_library (bx STATIC)



# Setup bx-core library
set (BX_CORE_SRCS
	"src/bx/core/log.cpp"
	"src/bx/core/memory.cpp"
	"src/bx/core/profiler.cpp"
	"src/bx/core/time.cpp"
	"src/bx/core/uuid.cpp"

	"src/bx/core/math/math.cpp"
)

set (BX_CORE_LIBS cereal fmt glm file_embed)

add_library (bx-core STATIC ${BX_CORE_SRCS})
target_link_libraries (bx-core ${BX_CORE_LIBS})
target_include_directories (bx-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries (bx bx-core)



# Setup bx-platform library
set (BX_PLATFORM_SRCS
	"src/bx/platform/audio.cpp"
	"src/bx/platform/graphics.cpp"
	"src/bx/platform/imgui.cpp"
	"src/bx/platform/input.cpp"
	"src/bx/platform/window.cpp"
)

set (BX_PLATFORM_LIBS bx-core imgui)

add_library (bx-platform STATIC ${BX_PLATFORM_SRCS})
target_link_libraries (bx-platform ${BX_PLATFORM_LIBS})
target_include_directories (bx-platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries (bx bx-platform)

# Setup platform backend
if (BX_PLATFORM_BACKEND STREQUAL "Default")
    target_link_libraries(bx-platform-common PRIVATE bx-platform)

	target_link_libraries (bx bx-platform-common)
endif()

# Setup window backend
if (BX_WINDOW_BACKEND STREQUAL "GLFW")
	# ImGui GLFW (TODO: temporary solution until a fully custom solution is made)
	add_library (imgui_glfw STATIC "extern/imgui/backends/imgui_impl_glfw.cpp")
	target_link_libraries(imgui_glfw imgui glfw)
	target_include_directories (imgui_glfw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

    target_link_libraries(bx-platform-glfw PRIVATE bx-platform imgui_glfw)

	target_link_libraries (bx bx-platform-glfw)
endif()

# Setup graphics backend
if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
	# ImGui OpenGL3 (TODO: temporary solution until a fully custom solution is made)
	add_library (imgui_opengl3 STATIC "extern/imgui/backends/imgui_impl_opengl3.cpp")
	target_link_libraries(imgui_opengl3 imgui)
	target_include_directories (imgui_opengl3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

	target_link_libraries(bx-platform-opengl PRIVATE bx-platform imgui_opengl3)

	target_link_libraries (bx bx-platform-opengl)
endif ()

# Setup audio backend
if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
	target_link_libraries(bx-platform-portaudio PRIVATE bx-platform)

	target_link_libraries (bx bx-platform-portaudio)
endif ()



# Setup bx-engine library
set (BX_ENGINE_SRCS
	"src/bx/engine/data.cpp"
	"src/bx/engine/ecs.cpp"
	"src/bx/engine/module.cpp"
	"src/bx/engine/object.cpp"
	"src/bx/engine/resource.cpp"
	"src/bx/engine/script.cpp"
)

set (BX_ENGINE_LIBS bx-core bx-platform wren)

add_library (bx-engine STATIC ${BX_ENGINE_SRCS})
target_link_libraries (bx-engine ${BX_ENGINE_LIBS})
target_include_directories (bx-engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries (bx bx-engine)

# Setup bx-editor library
if (BX_BUILD_EDITOR)
	set (BX_EDITOR_SRCS
		"src/bx/engine/editor/toolbar.cpp"
	)

	set (BX_EDITOR_LIBS bx-core imgui IconFontCppHeaders FontAwesome FreeFonts)

	add_library (bx-editor STATIC ${BX_EDITOR_SRCS})
	target_link_libraries (bx-editor ${BX_EDITOR_LIBS})
	target_include_directories (bx-editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

	target_link_libraries (bx bx-editor)
endif ()



# TODO: Clean this up
#if (MSVC)
#	target_compile_options(bx PUBLIC "/ZI")
#	target_link_options(bx PUBLIC "/INCREMENTAL")
#endif ()
#
#if (BX_INSTALL)
#	# Copy game contents to install dir
#	install(DIRECTORY ${CMAKE_SOURCE_DIR}/game DESTINATION bin)
#
#	# Package game in zip file of the installed files
#	#add_custom_target(install_and_package
#	#    COMMAND ${CMAKE_COMMAND} --install . --prefix ${CMAKE_INSTALL_PREFIX}
#	#    COMMAND ${CMAKE_COMMAND} -E tar cfv "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.zip" --format=zip -- "${CMAKE_INSTALL_PREFIX}/."
#	#    COMMENT "Creating distributable zip file"
#	#)
#
#	# Hide the console on installed builds
#	if (MSVC)
#		target_link_options(bx INTERFACE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
#	endif ()
#endif ()