cmake_minimum_required (VERSION 3.16)
message("CMake Version: ${CMAKE_VERSION}")

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
list (APPEND ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set (GLOBAL_CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
set (LOCAL_CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option (BX_BUILD_EDITOR "Build as editor binaries" ON)
option (BX_BUILD_TESTS "Build as editor binaries" ON)

set (BX_WINDOW_BACKEND "GLFW" CACHE STRING "Choose the window backend: GLFW")
set_property (CACHE BX_WINDOW_BACKEND PROPERTY STRINGS "GLFW")
message ("Window backend: ${BX_WINDOW_BACKEND}")

set (BX_GRAPHICS_BACKEND "OpenGL" CACHE STRING "Choose the graphics backend: OpenGL, OpenGLES")
set_property (CACHE BX_GRAPHICS_BACKEND PROPERTY STRINGS "OpenGL" "OpenGLES")
message ("Graphics backend: ${BX_GRAPHICS_BACKEND}")

set (BX_AUDIO_BACKEND "PortAudio" CACHE STRING "Choose the audio backend: PortAudio")
set_property (CACHE BX_AUDIO_BACKEND PROPERTY STRINGS "PortAudio")
message ("Audio backend: ${BX_AUDIO_BACKEND}")

set (BX_ONLINE_BACKEND "Steam" CACHE STRING "Choose the online backend: Steam")
set_property (CACHE BX_ONLINE_BACKEND PROPERTY STRINGS "Steam")
message ("Online backend: ${BX_ONLINE_BACKEND}")

# Engine library
add_library(Engine STATIC)

message ("CMake build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(Engine PUBLIC DEBUG_BUILD)
endif()

if (BX_BUILD_EDITOR)
	target_compile_definitions(Engine PUBLIC EDITOR_BUILD)
endif()

add_subdirectory (extern)

set (ENGINE_SRCS
	"src/engine/engine.cpp"
	"src/engine/log.cpp"
	"src/engine/math.cpp"
	"src/engine/profiler.cpp"
	"src/engine/uuid.cpp"
	"src/engine/file.cpp"

	"src/engine/window.cpp"
	"src/engine/graphics.cpp"
	"src/engine/audio.cpp"
	

	"src/framework/camera.cpp"
)

set (ENGINE_LIBS fmt stb glm wren rttr)

if (BX_BUILD_EDITOR)
	set (ENGINE_SRCS ${ENGINE_SRCS}
		"src/editor/editor.cpp"
		
	)
	set (ENGINE_LIBS ${ENGINE_LIBS} imgui implot ImGuizmo)
endif()

if (BX_WINDOW_BACKEND STREQUAL "GLFW")
	target_compile_definitions(Engine PUBLIC WINDOW_GLFW_BACKEND)
    set (ENGINE_SRCS ${ENGINE_SRCS} "src/engine/glfw/window_glfw.cpp")
    set (ENGINE_LIBS ${ENGINE_LIBS} glfw)

	if (BX_BUILD_EDITOR)
		set (ENGINE_SRCS ${ENGINE_SRCS} "src/editor/glfw/window_glfw.cpp")
	endif()

else ()
    message(FATAL_ERROR "Unknown window backend: ${BX_WINDOW_BACKEND}")
endif ()

if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
	target_compile_definitions(Engine PUBLIC GRAPHICS_OPENGL_BACKEND)
    set (ENGINE_SRCS ${ENGINE_SRCS} "src/engine/opengl/graphics_opengl.cpp")
    set (ENGINE_LIBS ${ENGINE_LIBS} glad)

	if (BX_BUILD_EDITOR)
		set (ENGINE_SRCS ${ENGINE_SRCS} "src/editor/opengl/graphics_opengl.cpp")
	endif()

elseif (BX_GRAPHICS_BACKEND STREQUAL "OpenGLES")
	target_compile_definitions(Engine PUBLIC GRAPHICS_OPENGLES_BACKEND)
    set (ENGINE_SRCS ${ENGINE_SRCS} "src/engine/opengl/graphics_opengles.cpp")
    set (ENGINE_LIBS ${ENGINE_LIBS} glad)

	if (BX_BUILD_EDITOR)
		set (ENGINE_SRCS ${ENGINE_SRCS} "src/editor/opengl/graphics_opengles.cpp")
	endif()

else ()
    message(FATAL_ERROR "Unknown graphics backend: ${BX_GRAPHICS_BACKEND}")
endif ()

if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
	target_compile_definitions(Engine PUBLIC AUDIO_PORTAUDIO_BACKEND)
    set (ENGINE_SRCS ${ENGINE_SRCS} "src/engine/portaudio/audio_portaudio.cpp")
    set (ENGINE_LIBS ${ENGINE_LIBS} portaudio)

	if (BX_BUILD_EDITOR)
		set (ENGINE_SRCS ${ENGINE_SRCS} "src/editor/portaudio/audio_portaudio.cpp")
	endif()

else ()
    message(FATAL_ERROR "Unknown audio backend: ${BX_AUDIO_BACKEND}")
endif ()

if (BX_ONLINE_BACKEND STREQUAL "Steam")
	if (STEAMWORKS_FOUND)
		target_compile_definitions(Engine PUBLIC ONLINE_STEAM_BACKEND)
		set (ENGINE_SRCS ${ENGINE_SRCS} "src/engine/steam/online_steam.cpp")
		set (ENGINE_LIBS ${ENGINE_LIBS} steamworks)

		if (BX_BUILD_EDITOR)
			set (ENGINE_SRCS ${ENGINE_SRCS} "src/editor/steam/online_steam.cpp")
		endif()

	else()
		target_compile_definitions(Engine PUBLIC ONLINE_DUMMY_BACKEND)
	endif()
else ()
    message(FATAL_ERROR "Unknown online backend: ${BX_ONLINE_BACKEND}")
endif ()

target_sources(Engine PRIVATE ${ENGINE_SRCS})
target_link_libraries (Engine PUBLIC ${ENGINE_LIBS})
target_include_directories (Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (BX_BUILD_TESTS)
	enable_testing()
	#add_subdirectory(tests)
endif ()