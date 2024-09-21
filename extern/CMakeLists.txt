# Cereal lib
set (BUILD_DOC OFF)
set (BUILD_SANDBOX OFF)
set (SKIP_PERFORMANCE_COMPARISON ON)

add_subdirectory (cereal)

# FMT lib
#set(FMT_MODULE ON)

add_subdirectory (fmt)

# GLM lib
add_subdirectory (glm)

# Wren lib
add_library (wren STATIC
	"wren/src/optional/wren_opt_meta.c"
	"wren/src/optional/wren_opt_random.c"
	"wren/src/vm/wren_compiler.c"
	"wren/src/vm/wren_core.c"
	"wren/src/vm/wren_debug.c"
	"wren/src/vm/wren_primitive.c"
	"wren/src/vm/wren_utils.c"
	"wren/src/vm/wren_value.c"
	"wren/src/vm/wren_vm.c"
)
target_include_directories (wren PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/wren/src/include ${CMAKE_CURRENT_SOURCE_DIR}/wren/src/optional ${CMAKE_CURRENT_SOURCE_DIR}/wren/src/vm)

# ImGui lib
set (imgui_src STATIC
	"imgui/imgui.cpp"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/misc/cpp/imgui_stdlib.cpp"
)
add_library (imgui STATIC ${imgui_src})
target_include_directories (imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

# Platform specific libs
if (BX_PLATFORM_BACKEND STREQUAL "Common")
	add_subdirectory (bx-platform-common)
endif ()

if (BX_WINDOW_BACKEND STREQUAL "GLFW")
	add_subdirectory (bx-platform-glfw)
endif ()

if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
    add_subdirectory (bx-platform-opengl)
endif ()

if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
    add_subdirectory (bx-platform-portaudio)
endif ()

if (BX_BUILD_EDITOR)
#	# STB lib
#	add_library (stb INTERFACE)
#	set_property(TARGET stb APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/stb)
#
#	# Assimp
#	add_subdirectory (assimp)
#
#
#	# ImGuizmo lib
#	add_library (ImGuizmo STATIC "ImGuizmo/GraphEditor.cpp" "ImGuizmo/ImCurveEdit.cpp" "ImGuizmo/ImGradient.cpp" "ImGuizmo/ImGuizmo.cpp" "ImGuizmo/ImSequencer.cpp")
#	target_link_libraries (ImGuizmo imgui)
#	target_include_directories (ImGuizmo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)

	# ImPlot lib
	add_library (implot STATIC "implot/implot.cpp" "implot/implot_demo.cpp" "implot/implot_items.cpp")
	target_link_libraries (implot imgui)
	target_include_directories (implot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/implot)

	# IconFontCppHeaders lib
	add_library (IconFontCppHeaders INTERFACE)
	set_property (TARGET IconFontCppHeaders APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/IconFontCppHeaders)

	# Font-Awesome
	add_library (FontAwesome INTERFACE)
	target_compile_definitions (FontAwesome INTERFACE FONT_AWESOME_6_BRANDS_REGULAR_400="${CMAKE_CURRENT_SOURCE_DIR}/Font-Awesome/otfs/Font Awesome 6 Brands-Regular-400.otf")
	target_compile_definitions (FontAwesome INTERFACE FONT_AWESOME_6_FREE_REGULAR_400="${CMAKE_CURRENT_SOURCE_DIR}/Font-Awesome/otfs/Font Awesome 6 Free-Regular-400.otf")
	target_compile_definitions (FontAwesome INTERFACE FONT_AWESOME_6_FREE_SOLID_900="${CMAKE_CURRENT_SOURCE_DIR}/Font-Awesome/otfs/Font Awesome 6 Free-Solid-900.otf")

	# free-fonts
	add_library (FreeFonts INTERFACE)
	target_compile_definitions (FreeFonts INTERFACE FREE_FONTS_DROID_SANS="${CMAKE_CURRENT_SOURCE_DIR}/free-fonts/Droid/Droid Sans/TTF/DroidSans.ttf")
endif ()

# GoogleTest
set (gtest_force_shared_crt ON)
add_subdirectory (googletest)