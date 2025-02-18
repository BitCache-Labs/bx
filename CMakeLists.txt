cmake_minimum_required (VERSION 3.16)
message("CMake Version: ${CMAKE_VERSION}")

option (BX_BUILD_EDITOR "Build as editor binaries" ON)
option (BX_BUILD_TESTS "Build test binaries" ON)
option (BX_INSTALL "Install binaries" ON)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
list (APPEND ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set (GLOBAL_CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
set (LOCAL_CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set (BX_WINDOW_BACKEND "GLFW" CACHE STRING "Choose the window backend: GLFW")
set_property (CACHE BX_WINDOW_BACKEND PROPERTY STRINGS "GLFW")
message ("Window backend: ${BX_WINDOW_BACKEND}")

set (BX_GRAPHICS_BACKEND "OpenGL" CACHE STRING "Choose the graphics backend: OpenGL, OpenGLES")
set_property (CACHE BX_GRAPHICS_BACKEND PROPERTY STRINGS "OpenGL" "OpenGLES")
message ("Graphics backend: ${BX_GRAPHICS_BACKEND}")

set (BX_AUDIO_BACKEND "PortAudio" CACHE STRING "Choose the audio backend: PortAudio")
set_property (CACHE BX_AUDIO_BACKEND PROPERTY STRINGS "PortAudio")
message ("Audio backend: ${BX_AUDIO_BACKEND}")

set (BX_SCRIPT_BACKEND "Wren" CACHE STRING "Choose the script backend: Wren")
set_property (CACHE BX_SCRIPT_BACKEND PROPERTY STRINGS "Wren")
message ("Script backend: ${BX_SCRIPT_BACKEND}")

set (BX_ONLINE_BACKEND "Steam" CACHE STRING "Choose the online backend: Steam")
set_property (CACHE BX_ONLINE_BACKEND PROPERTY STRINGS "Steam")
message ("Online backend: ${BX_ONLINE_BACKEND}")

set (BX_ENGINE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# bx engine library
add_library(bx-engine STATIC)

if (BX_INSTALL)
	target_compile_definitions (bx-engine PUBLIC GAME_PATH="game")
	target_compile_definitions (bx-engine PUBLIC ENGINE_PATH="game/assets")
else()
	target_compile_definitions (bx-engine PUBLIC GAME_PATH="${BX_GAME_PATH}")
	target_compile_definitions (bx-engine PUBLIC ENGINE_PATH="${BX_ENGINE_PATH}")
endif ()

message ("CMake build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions (bx-engine PUBLIC DEBUG_BUILD)
endif()

if (BX_BUILD_EDITOR)
	target_compile_definitions (bx-engine PUBLIC EDITOR_BUILD)
endif()

add_subdirectory (extern)

set (BX_ENGINE_SRCS
	# Core
	"src/engine/engine.cpp"
	"src/engine/log.cpp"
	"src/engine/math.cpp"
	"src/engine/profiler.cpp"
	"src/engine/uuid.cpp"
	"src/engine/debug.cpp"
	"src/engine/serial.cpp"

	# Modules
	"src/engine/file.cpp"
	"src/engine/window.cpp"
	"src/engine/graphics.cpp"
	"src/engine/audio.cpp"
	"src/engine/script.cpp"

	"src/engine/common/common_file.cpp"

	# Framework (TODO: Put into new lib bx-framework)
	"src/framework/world.cpp"
	"src/framework/camera.cpp"
)

set (BX_ENGINE_LIBS fmt stb glm rttr cereal rapidjson)

if (BX_BUILD_EDITOR)
	set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS}
		"src/editor/imgui_ex.cpp"
		"src/editor/editor.cpp"
		"src/editor/console.cpp"
		"src/editor/profiler.cpp"
		"src/editor/settings.cpp"
		"src/editor/assets.cpp"
		
		"src/framework/editor/world_editor.cpp"
		"src/framework/editor/node_editor.cpp"
		"src/framework/editor/code_editor.cpp"
	)
	set (BX_ENGINE_LIBS ${BX_ENGINE_LIBS} imgui implot ImGuizmo IconFontCppHeaders)
endif()

# Detect module implementation
if (BX_WINDOW_BACKEND STREQUAL "GLFW")
	target_compile_definitions(bx-engine PUBLIC WINDOW_GLFW_BACKEND)
    set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/engine/glfw/window_glfw.cpp")
    set (BX_ENGINE_LIBS ${BX_ENGINE_LIBS} glfw)

	if (BX_BUILD_EDITOR)
		set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/editor/glfw/window_glfw.cpp")
	endif()

else ()
    message(FATAL_ERROR "Unknown window backend: ${BX_WINDOW_BACKEND}")
endif ()

if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
	target_compile_definitions(bx-engine PUBLIC GRAPHICS_OPENGL_BACKEND)
    set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/engine/opengl/graphics_opengl.cpp")
    set (BX_ENGINE_LIBS ${BX_ENGINE_LIBS} glad)

	if (BX_BUILD_EDITOR)
		set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/editor/opengl/graphics_opengl.cpp")
	endif()

elseif (BX_GRAPHICS_BACKEND STREQUAL "OpenGLES")
	target_compile_definitions(bx-engine PUBLIC GRAPHICS_OPENGLES_BACKEND)
    set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/engine/opengl/graphics_opengles.cpp")
    set (BX_ENGINE_LIBS ${BX_ENGINE_LIBS} glad)

	if (BX_BUILD_EDITOR)
		set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/editor/opengl/graphics_opengles.cpp")
	endif()

else ()
    message(FATAL_ERROR "Unknown graphics backend: ${BX_GRAPHICS_BACKEND}")
endif ()

if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
	target_compile_definitions(bx-engine PUBLIC AUDIO_PORTAUDIO_BACKEND)
    set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/engine/portaudio/audio_portaudio.cpp")
    set (BX_ENGINE_LIBS ${BX_ENGINE_LIBS} portaudio)

	if (BX_BUILD_EDITOR)
		set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/editor/portaudio/audio_portaudio.cpp")
	endif()

else ()
    message(FATAL_ERROR "Unknown audio backend: ${BX_AUDIO_BACKEND}")
endif ()

if (BX_SCRIPT_BACKEND STREQUAL "Wren")
	target_compile_definitions(bx-engine PUBLIC SCRIPT_WREN_BACKEND)
    set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/engine/wren/script_wren.cpp")
    set (BX_ENGINE_LIBS ${BX_ENGINE_LIBS} wren)

	#if (BX_BUILD_EDITOR)
	#	set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/editor/portaudio/audio_portaudio.cpp")
	#endif()

else ()
    message(FATAL_ERROR "Unknown script backend: ${BX_SCRIPT_BACKEND}")
endif ()

if (BX_ONLINE_BACKEND STREQUAL "Steam")
	if (STEAMWORKS_FOUND)
		target_compile_definitions(bx-engine PUBLIC ONLINE_STEAM_BACKEND)
		set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/engine/steam/online_steam.cpp")
		set (BX_ENGINE_LIBS ${BX_ENGINE_LIBS} steamworks)

		if (BX_BUILD_EDITOR)
			set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/editor/steam/online_steam.cpp")
		endif()

	else()
		set (BX_ENGINE_SRCS ${BX_ENGINE_SRCS} "src/engine/online.cpp")
	endif()
else ()
    message(FATAL_ERROR "Unknown online backend: ${BX_ONLINE_BACKEND}")
endif ()

target_sources(bx-engine PRIVATE ${BX_ENGINE_SRCS})
target_link_libraries (bx-engine PUBLIC ${BX_ENGINE_LIBS})
target_include_directories (bx-engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (BX_BUILD_TESTS)
	enable_testing()
	#add_subdirectory(tests)
endif ()

# -------------------
# Game executable
# -------------------

if (BX_BUILD_EDITOR)
	set (BX_GAME_SRCS ${BX_GAME_SRCS} ${BX_GAME_EDITOR_SRCS})
endif()

# We need the -WHOLEARCHIVE to avoid static compile optimize away
set (BX_GAME_LIBS ${BX_GAME_LIBS} bx-engine -WHOLEARCHIVE:$<TARGET_FILE:bx-engine>)

add_library (bx-game STATIC ${BX_GAME_SRCS})
target_link_libraries (bx-game ${BX_GAME_LIBS})
target_include_directories (bx-game PUBLIC ${BX_GAME_INCLUDE_PATH})

if(WIN32)
	add_executable (${BX_GAME_NAME} ${BX_GAME_ICON_PATH} ${BX_GAME_RUNTIME_SRC})
#elseif(APPLE)
#elseif(UNIX)
else()
	add_executable (${BX_GAME_NAME} ${BX_GAME_RUNTIME_SRC})
endif()

target_link_libraries (${BX_GAME_NAME} bx-game -WHOLEARCHIVE:$<TARGET_FILE:bx-game>)

if (WIN32)
	# We need to copy steam dll to the exe bin on windows
	if (BX_ONLINE_BACKEND STREQUAL "Steam" AND STEAMWORKS_FOUND)
		# Iterate through all executables and copy the DLL to each one
		add_custom_command(
			TARGET ${BX_GAME_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${STEAMWORKS_DLL_PATH} $<TARGET_FILE_DIR:${BX_GAME_NAME}>
			COMMENT "Copying Steamworks DLL to the executable output directory"
		)
	endif()
endif()

# Install
if (BX_INSTALL)
	install(TARGETS ${BX_GAME_NAME}
	  RUNTIME DESTINATION bin       # For executables
	  LIBRARY DESTINATION lib       # For shared libraries
	  ARCHIVE DESTINATION lib       # For static libraries
	)

	if (WIN32)
	  if (BX_ONLINE_BACKEND STREQUAL "Steam" AND STEAMWORKS_FOUND)
		install(
		  FILES ${STEAMWORKS_DLL_PATH}
		  DESTINATION bin
		)
	  endif()
	endif()

	# Assets
	install(
	  DIRECTORY ${BX_GAME_PATH}/
	  DESTINATION bin/game
	)

	install(
	  DIRECTORY ${BX_ENGINE_PATH}/
	  DESTINATION bin/game/assets
	)
endif()