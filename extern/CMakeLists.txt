# RTTR lib
set(BUILD_RTTR_DYNAMIC ON CACHE BOOL "Build RTTR as a shared library (ON) or static library (OFF)")
set(BUILD_UNIT_TESTS OFF CACHE BOOL "Build RTTR unit tests")
set(BUILD_STATIC OFF CACHE BOOL "Build RTTR as a static library")
set(BUILD_WITH_STATIC_RUNTIME_LIBS OFF CACHE BOOL "Use static runtime libraries for RTTR")
set(BUILD_WITH_RTTI OFF CACHE BOOL "Disable RTTI support in RTTR")
set(BUILD_BENCHMARKS OFF CACHE BOOL "Disable building RTTR benchmarks")
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable building RTTR examples")
set(BUILD_DOCUMENTATION OFF CACHE BOOL "Disable building RTTR documentation")
set(BUILD_INSTALLER OFF CACHE BOOL "Disable building RTTR installer")
set(BUILD_PACKAGE OFF CACHE BOOL "Disable building RTTR package")
set(USE_PCH ON CACHE BOOL "Enable precompiled headers for RTTR")
set(CUSTOM_DOXYGEN_STYLE OFF CACHE BOOL "Disable custom Doxygen styling for RTTR documentation")
set(BUILD_WEBSITE_DOCU OFF CACHE BOOL "Disable building website documentation for RTTR")

add_subdirectory(rttr)

# Create an alias for the rttr library
if (BUILD_STATIC)
    add_library(rttr ALIAS rttr_core_lib)
else ()
    add_library(rttr ALIAS rttr_core)

    # Define the path to the DLL and the output directory
    set(RTTR_DLL_PATH "$<TARGET_FILE:rttr_core>")

    # Define a custom target to copy the DLL
    add_custom_target(copy_rttr_dll ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RTTR_DLL_PATH}"
            "${CMAKE_BINARY_DIR}"
        COMMENT "Copying RTTR DLL to the output directory"
        VERBATIM
    )

	if (BX_INSTALL)
		install(FILES "${RTTR_DLL_PATH}"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"  # Or another relevant subdirectory, like lib or bin
		COMPONENT runtime
)
	endif()
endif()

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
set (imgui_srcs
	"imgui/imgui.cpp"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/misc/cpp/imgui_stdlib.cpp"
)

#if (BUILD_STATIC)
	add_library (imgui STATIC ${imgui_srcs})

#else ()
#    add_library (imgui SHARED ${imgui_srcs})
#
#    # Define the path to the DLL and the output directory
#    set(IMGUI_DLL_PATH "$<TARGET_FILE:imgui>")
#
#    # Define a custom target to copy the DLL
#    add_custom_target(copy_imgui_dll ALL
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            "${IMGUI_DLL_PATH}"
#            "${CMAKE_BINARY_DIR}"
#        COMMENT "Copying IMGUI DLL to the output directory"
#        VERBATIM
#    )
#endif()

target_include_directories (imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

# BX common platform libs
add_subdirectory (bx-platform-common)

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

# Set the output directory for bx-editor and bx
#set_target_properties(bx PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
#    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
#)

# GoogleTest
set (gtest_force_shared_crt ON)
add_subdirectory (googletest)