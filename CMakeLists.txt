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

# TODO: Perhaps these are better suited to be here instead of bx-framework so bx can still be used
## Define options for window backend
#set (BX_WINDOW_BACKEND "GLFW" CACHE STRING "Choose the window backend: GLFW")
#set_property (CACHE BX_WINDOW_BACKEND PROPERTY STRINGS "GLFW" "Custom")
#message ("BX window backend: ${BX_WINDOW_BACKEND}")
#
#if (BX_WINDOW_BACKEND STREQUAL "GLFW")
#    message(STATUS "Using GLFW as the window backend")
#    add_compile_definitions (BX_WINDOW_GLFW_BACKEND)
#
#elseif (BX_WINDOW_BACKEND STREQUAL "Custom")
#    message(STATUS "Using a custom window backend")
#    add_compile_definitions (BX_WINDOW_CUSTOM_BACKEND)
#
#else ()
#    message(FATAL_ERROR "Unknown window backend: ${BX_WINDOW_BACKEND}")
#endif ()
#
## Define options for graphics backend
#set (BX_GRAPHICS_BACKEND "OpenGL" CACHE STRING "Choose the graphics backend: OpenGL, OpenGLES, Vulkan")
#set_property (CACHE BX_GRAPHICS_BACKEND PROPERTY STRINGS "OpenGL" "OpenGLES" "Vulkan" "Custom")
#message ("BX graphics backend: ${BX_GRAPHICS_BACKEND}")
#
#if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
#    message(STATUS "Using OpenGL as the graphics backend")
#    add_compile_definitions (BX_GRAPHICS_OPENGL_BACKEND)
#
#elseif (BX_GRAPHICS_BACKEND STREQUAL "OpenGLES")
#    message(STATUS "Using OpenGLES as the graphics backend")
#    add_compile_definitions (BX_GRAPHICS_OPENGLES_BACKEND)
#
#elseif (BX_GRAPHICS_BACKEND STREQUAL "Vulkan")
#    message(STATUS "Using Vulkan as the graphics backend")
#    add_compile_definitions (BX_GRAPHICS_VULKAN_BACKEND)
#
#elseif (BX_GRAPHICS_BACKEND STREQUAL "Custom")
#    message(STATUS "Using a custom graphics backend")
#    add_compile_definitions (BX_GRAPHICS_CUSTOM_BACKEND)
#
#else ()
#    message(FATAL_ERROR "Unknown graphics backend: ${GRAPHICS_BACKEND}")
#endif ()
#
## Define options for audio backend
#set (BX_AUDIO_BACKEND "PortAudio" CACHE STRING "Choose the audio backend: PortAudio")
#set_property (CACHE BX_AUDIO_BACKEND PROPERTY STRINGS "PortAudio" "Custom")
#message ("BX audio backend: ${BX_AUDIO_BACKEND}")
#
#if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
#    message(STATUS "Using PortAudio as the audio backend")
#    add_compile_definitions (BX_AUDIO_PORTAUDIO_BACKEND)
#
#elseif (BX_AUDIO_BACKEND STREQUAL "Custom")
#    message(STATUS "Using a custom audio backend")
#    add_compile_definitions (BX_AUDIO_CUSTOM_BACKEND)
#
#else ()
#    message(FATAL_ERROR "Unknown audio backend: ${WINDOW_BACKEND}")
#endif ()

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



# Setup bx-core library
set (BX_CORE_SRCS
	"src/bx/core/data.cpp"
	"src/bx/core/file.cpp"
	"src/bx/core/input.cpp"
	"src/bx/core/log.cpp"
	"src/bx/core/memory.cpp"
	"src/bx/core/module.cpp"
	"src/bx/core/object.cpp"
	"src/bx/core/profiler.cpp"
	"src/bx/core/resource.cpp"
	"src/bx/core/time.cpp"
	"src/bx/core/uuid.cpp"

	# TODO: These should be in bx-platform-common
	"src/bx/core/platform/file.cpp"
	"src/bx/core/platform/thread.cpp"

	# TODO: Check if this is still necessary
	"src/bx/core/platform/audio.cpp"
	"src/bx/core/platform/graphics.cpp"
	"src/bx/core/platform/imgui.cpp"
	"src/bx/core/platform/thread.cpp"
	"src/bx/core/platform/window.cpp"
)

set (BX_CORE_LIBS cereal fmt file_embed)

add_library (bx-core STATIC ${BX_CORE_SRCS})
target_link_libraries (bx-core ${BX_CORE_LIBS})
target_include_directories (bx-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)



# Setup bx-ecs library
set (BX_ECS_SRCS
	"src/bx/ecs/ecs.cpp"
)

set (BX_ECS_LIBS bx-core)

add_library (bx-ecs STATIC ${BX_ECS_SRCS})
target_link_libraries (bx-ecs ${BX_ECS_LIBS})
target_include_directories (bx-ecs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)



# Setup bx-math library
set (BX_MATH_SRCS
	"src/bx/math/math.cpp"
)

set (BX_MATH_LIBS bx-core glm)

add_library (bx-math STATIC ${BX_MATH_SRCS})
target_link_libraries (bx-math ${BX_MATH_LIBS})
target_include_directories (bx-math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)



# Setup bx-script library
set (BX_SCRIPT_SRCS
	"src/bx/script/script.cpp"
)

set (BX_SCRIPT_LIBS bx-core wren)

add_library (bx-script STATIC ${BX_SCRIPT_SRCS})
target_link_libraries (bx-script ${BX_SCRIPT_LIBS})
target_include_directories (bx-script PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)



# Setup bx stack library
add_library (bx STATIC)
target_link_libraries (bx bx-core bx-ecs bx-math bx-script)



# Setup bx-editor library
if (BX_BUILD_EDITOR)
	set (BX_EDITOR_SRCS
		"src/bx/editor/core/toolbar.cpp"
	)

	set (BX_EDITOR_LIBS bx-core imgui IconFontCppHeaders FontAwesome FreeFonts)#stb assimp implot ImGuizmo IconFontCppHeaders FontAwesome FreeFonts)

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