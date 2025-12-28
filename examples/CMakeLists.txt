add_executable(bx_example_basic "basic/main.cpp")
target_link_libraries(bx_example_basic bx_app)

target_compile_definitions(bx_example_basic PRIVATE BX_EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}")