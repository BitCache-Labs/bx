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
option (BX_BUILD_DEMO "Build the demo game" ON)

# Setup bx library
set (BX_SRCS
	"src/bx/core/file.cpp"
	"src/bx/core/log.cpp"
	"src/bx/core/memory.cpp"
	"src/bx/core/module.cpp"
	"src/bx/core/object.cpp"
	"src/bx/core/profiler.cpp"
	"src/bx/core/serial.cpp"
	"src/bx/core/thread.cpp"
	"src/bx/core/time.cpp"
	"src/bx/core/uuid.cpp"

	"src/bx/math/math.cpp"

	"src/bx/platform/audio.cpp"
	"src/bx/platform/graphics.cpp"
	"src/bx/platform/input.cpp"
	"src/bx/platform/network.cpp"
	"src/bx/platform/window.cpp"

	"src/bx/engine/application.cpp"
	"src/bx/engine/data.cpp"
	"src/bx/engine/ecs.cpp"
	"src/bx/engine/imgui.cpp"
	"src/bx/engine/plugin.cpp"
	"src/bx/engine/resource.cpp"
	"src/bx/engine/script.cpp"
)

add_library (bx SHARED ${BX_SRCS} "src/bx/core/module.cpp")
target_include_directories (bx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(bx PRIVATE BUILD_SHARED)
target_compile_definitions(bx PUBLIC BX_API=API)

if (MSVC)
    target_compile_options(bx PRIVATE /bigobj)
endif()

if (BX_INSTALL)
	set (PROJECT_PATH_DIR ".")
else ()
	#set (PROJECT_PATH_DIR "${CMAKE_SOURCE_DIR}")
	set (PROJECT_PATH_DIR "${CMAKE_BINARY_DIR}")
endif ()

#target_compile_definitions(bx PUBLIC BX_PROJECT_PATH="${PROJECT_PATH_DIR}")
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

# Editor sources and libraries
if (BX_BUILD_EDITOR)
	target_compile_definitions (bx PUBLIC BX_EDITOR_BUILD)

    target_sources(bx PRIVATE
		"src/bx/editor/command.cpp"
		"src/bx/editor/selection.cpp"
		"src/bx/editor/toolbar.cpp"
		"src/bx/editor/view.cpp"
		"src/bx/editor/assets.cpp"
		
		"src/bx/editor/views/assets_view.cpp"
		#"src/bx/editor/views/console_view.cpp"
		#"src/bx/editor/views/data_view.cpp"
		#"src/bx/editor/views/entity_view.cpp"
		"src/bx/editor/views/profiler_view.cpp"
		#"src/bx/editor/views/script_view.cpp"
		#"src/bx/editor/views/settings_view.cpp"

		"src/bx/editor/build/compiler.cpp"
	)
    target_link_libraries(bx PUBLIC implot IconFontCppHeaders FontAwesome FreeFonts)

	if(WIN32)
		add_executable (bx-editor "src/bx/runtime/editor.cpp" "resources/bx.rc")
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

	if (BX_BUILD_EDITOR)
		# Hide the console on installed builds
		if (MSVC)
			target_link_options(bx-editor PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
		endif ()
	endif ()
endif ()

if (BX_BUILD_DEMO)
	add_subdirectory(demo)
	target_compile_definitions (bx-editor PUBLIC PROJECT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/demo")
endif()