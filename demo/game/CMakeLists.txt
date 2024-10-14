cmake_minimum_required (VERSION 3.22)

project (demo)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (GAME_SRCS
	"src/components/sprite.cpp"
	"src/systems/drawer.cpp"
	"src/game.cpp"
)

if (BX_BUILD_EDITOR)
	set (GAME_SRCS ${GAME_SRCS}
		"src/editor/level.cpp"
	)
endif()

add_library(game SHARED ${GAME_SRCS})
target_link_libraries(game PRIVATE bx)
target_include_directories(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(game PRIVATE BUILD_SHARED)
target_compile_definitions(game PUBLIC GAME_API=API)

# Set output directory for the game DLL
set (${PROJECT_NAME}_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/..")
set_target_properties(game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${${PROJECT_NAME}_LIBS}/libs"
    LIBRARY_OUTPUT_DIRECTORY "${${PROJECT_NAME}_LIBS}/libs"
)