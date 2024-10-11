cmake_minimum_required (VERSION 3.22)
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Compiler Version: ${CMAKE_C_COMPILER_VERSION}")

project (bx)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

option (BX_BUILD_EDITOR "Build as editor binaries" ON)
option (BX_INSTALL "Install binaries" OFF)
option (BX_BUILD_TESTS "Build the test binaries" ON)

# Define options for platform backend
set (BX_PLATFORM_BACKEND "Common" CACHE STRING "Choose the platform backend: Common")
set_property (CACHE BX_PLATFORM_BACKEND PROPERTY STRINGS "Common" "Custom")
message ("BX platform backend: ${BX_PLATFORM_BACKEND}")

# Define options for window backend
set (BX_WINDOW_BACKEND "GLFW" CACHE STRING "Choose the window backend: GLFW")
set_property (CACHE BX_WINDOW_BACKEND PROPERTY STRINGS "GLFW" "Custom")
message ("BX window backend: ${BX_WINDOW_BACKEND}")

# Define options for graphics backend
set (BX_GRAPHICS_BACKEND "OpenGL" CACHE STRING "Choose the graphics backend: OpenGL, OpenGLES, Vulkan")
set_property (CACHE BX_GRAPHICS_BACKEND PROPERTY STRINGS "OpenGL" "OpenGLES" "Vulkan" "Custom")
message ("BX graphics backend: ${BX_GRAPHICS_BACKEND}")

# Define options for audio backend
set (BX_AUDIO_BACKEND "PortAudio" CACHE STRING "Choose the audio backend: PortAudio")
set_property (CACHE BX_AUDIO_BACKEND PROPERTY STRINGS "PortAudio" "Custom")
message ("BX audio backend: ${BX_AUDIO_BACKEND}")

# Setup bx library
set (BX_SRCS
	"src/bx/core/file.cpp"
	"src/bx/core/log.cpp"
	"src/bx/core/memory.cpp"
	"src/bx/core/object.cpp"
	"src/bx/core/profiler.cpp"
	"src/bx/core/thread.cpp"
	"src/bx/core/time.cpp"
	"src/bx/core/uuid.cpp"

	"src/bx/math/math.cpp"

	#"src/bx/platform/audio.cpp"
	"src/bx/platform/graphics.cpp"
	#"src/bx/platform/imgui.cpp"
	#"src/bx/platform/input.cpp"
	"src/bx/platform/window.cpp"

	"src/bx/engine/data.cpp"
	"src/bx/engine/ecs.cpp"
	
	"src/bx/engine/resource.cpp"
	"src/bx/engine/application.cpp"
	"src/bx/engine/script.cpp"
	"src/bx/engine/plugin.cpp"
	"src/bx/engine/application.cpp"
)

add_library (bx SHARED ${BX_SRCS} "src/bx/core/module.cpp")
target_include_directories (bx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(bx PRIVATE BX_EXPORT)

if (MSVC)
    target_compile_options(bx PRIVATE /bigobj)
endif()

if (BX_INSTALL)
	set (PROJECT_PATH_DIR ".")
else ()
	set (PROJECT_PATH_DIR "${CMAKE_SOURCE_DIR}")
endif ()

target_compile_definitions(bx PUBLIC BX_PROJECT_PATH="${PROJECT_PATH_DIR}")
target_compile_definitions(bx PRIVATE BX_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

# TODO: Use Platform.hpp for the future
message ("CMake system name: ${CMAKE_SYSTEM_NAME}")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions (bx PUBLIC BX_PLATFORM_WINDOWS)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions (bx PUBLIC BX_PLATFORM_LINUX)
endif ()

message ("CMake build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions (bx PUBLIC BX_DEBUG_BUILD)

else ()
    target_compile_definitions (bx PUBLIC BX_RELEASE_BUILD)
endif ()
# ---------------------------------------

# External libraries

add_subdirectory (extern)
#set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
#add_subdirectory (wren)

set (BX_LIBS rttr cereal fmt glm imgui wren)# file_embed)
target_link_libraries (bx PUBLIC ${BX_LIBS})

# Setup platform backend
#if (BX_PLATFORM_BACKEND STREQUAL "Common")
#    message(STATUS "Using the common platform backend")
#	target_compile_definitions(bx PUBLIC BX_PLATFORM_COMMON_BACKEND)
#    target_link_libraries(bx-platform-common PRIVATE bx)
#	target_link_libraries(bx PUBLIC bx-platform-common)
#
#elseif (BX_PLATFORM_BACKEND STREQUAL "Custom")
#    message(STATUS "Using a custom platform backend")
#	target_compile_definitions(bx PUBLIC BX_PLATFORM_CUSTOM_BACKEND)
#
#else ()
#    message(FATAL_ERROR "Unknown platform backend: ${BX_PLATFORM_BACKEND}")
#endif ()
#

# Configure the window backend based on selection
if (BX_WINDOW_BACKEND STREQUAL "GLFW")
    message(STATUS "Using GLFW as the window backend")
    target_compile_definitions(bx PUBLIC BX_WINDOW_GLFW_BACKEND)

elseif (BX_WINDOW_BACKEND STREQUAL "Custom")
    message(STATUS "Using a custom window backend")
    target_compile_definitions(bx PUBLIC BX_WINDOW_CUSTOM_BACKEND)

else ()
    message(FATAL_ERROR "Unknown window backend: ${BX_WINDOW_BACKEND}")
endif ()

#
## Setup graphics backend
#if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
#    message(STATUS "Using OpenGL as the graphics backend")
#    target_compile_definitions(bx PUBLIC BX_GRAPHICS_OPENGL_BACKEND)
#
#	# ImGui OpenGL3 (TODO: temporary solution until a fully custom solution is made)
#	add_library (imgui_opengl3 STATIC "extern/imgui/backends/imgui_impl_opengl3.cpp")
#	target_link_libraries(imgui_opengl3 imgui)
#
#	target_link_libraries(bx-platform-opengl PRIVATE bx imgui_opengl3)
#	target_link_libraries (bx PUBLIC bx-platform-opengl)
#
##elseif (BX_GRAPHICS_BACKEND STREQUAL "OpenGLES")
##    message(STATUS "Using OpenGLES as the graphics backend")
##    target_compile_definitions(bx-platform PUBLIC BX_GRAPHICS_OPENGLES_BACKEND)
##
##elseif (BX_GRAPHICS_BACKEND STREQUAL "Vulkan")
##    message(STATUS "Using Vulkan as the graphics backend")
##    target_compile_definitions(bx-platform PUBLIC BX_GRAPHICS_VULKAN_BACKEND)
#
#elseif (BX_GRAPHICS_BACKEND STREQUAL "Custom")
#    message(STATUS "Using a custom graphics backend")
#    target_compile_definitions(bx PUBLIC BX_GRAPHICS_CUSTOM_BACKEND)
#
#else ()
#    message(FATAL_ERROR "Unknown graphics backend: ${GRAPHICS_BACKEND}")
#endif ()
#
## Setup audio backend
#if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
#    message(STATUS "Using PortAudio as the audio backend")
#    target_compile_definitions(bx PUBLIC BX_AUDIO_PORTAUDIO_BACKEND)
#	target_link_libraries(bx-platform-portaudio PRIVATE bx)
#	target_link_libraries(bx PUBLIC bx-platform-portaudio)
#
#elseif (BX_AUDIO_BACKEND STREQUAL "Custom")
#    message(STATUS "Using a custom audio backend")
#    target_compile_definitions(bx PUBLIC BX_AUDIO_CUSTOM_BACKEND)
#
#else ()
#    message(FATAL_ERROR "Unknown audio backend: ${WINDOW_BACKEND}")
#endif ()

# Set the output directory for bx-editor and bx
#set_target_properties(bx PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
#    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
#)

# Editor sources and libraries
if (BX_BUILD_EDITOR)
	target_compile_definitions (bx PUBLIC BX_EDITOR_BUILD)

    #target_sources(bx PRIVATE
	#	#"src/bx/editor/command.cpp"
	#	#"src/bx/editor/selection.cpp"
	#	#"src/bx/editor/view.cpp"
	#	#
	#	#"src/bx/editor/assets.cpp"
	#	#"src/bx/editor/asset_importer.cpp"
	#	#"src/bx/editor/toolbar.cpp"
	#
	#	#"src/bx/editor/views/assets_view.cpp"
	#	#"src/bx/editor/views/console_view.cpp"
	#	#"src/bx/editor/views/data_view.cpp"
	#	#"src/bx/editor/views/entity_view.cpp"
	#	#"src/bx/editor/views/profiler_view.cpp"
	#	#"src/bx/editor/views/script_view.cpp"
	#	#"src/bx/editor/views/settings_view.cpp"
	#)
    #target_link_libraries(bx PUBLIC implot IconFontCppHeaders FontAwesome FreeFonts)

	if(WIN32)
		add_executable (bx-editor "src/bx/runtime/editor.cpp" "resources/bx.rc" "src/bx/core/module.cpp")
	#elseif(APPLE)
	#elseif(UNIX)
	else()
		add_executable (bx-editor "src/bx/runtime/editor.cpp")
	endif()

	target_link_libraries (bx-editor bx)

	#set_target_properties(bx-editor PROPERTIES
	#	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
	#	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
	#)
endif()

if (BX_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif ()

if (BX_INSTALL)
	install(TARGETS bx bx-editor
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

	# TODO: Implement this properly
#	install(TARGETS ${PROJECT_NAME} DESTINATION bin)
#
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
endif ()