# Ctti lib
add_subdirectory (ctti)

# Wnaabi lib
#add_subdirectory (wnaabi)
add_library (wnaabi INTERFACE)
set_property(TARGET wnaabi APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/wnaabi/include)

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

# Portaudio lib
add_subdirectory (portaudio)

# VulkanMemoryAllocator lib
add_subdirectory (VulkanMemoryAllocator)

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

# Box2D lib
set (BOX2D_BUILD_UNIT_TESTS OFF CACHE INTERNAL "")
set (BOX2D_BUILD_TESTBED OFF CACHE INTERNAL "")
set (BOX2D_BUILD_DOCS OFF CACHE INTERNAL "")
set (BOX2D_USER_SETTINGS OFF CACHE INTERNAL "")
set (BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

add_subdirectory (box2d)

# Bullet3 lib
set (USE_DOUBLE_PRECISION OFF CACHE INTERNAL "")
set (USE_GRAPHICAL_BENCHMARK OFF CACHE INTERNAL "")
set (USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD ON CACHE INTERNAL "")
set (USE_OPENVR OFF CACHE INTERNAL "")
set (BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set (ENABLE_VHACD OFF CACHE INTERNAL "")
set (BULLET2_MULTITHREADING OFF CACHE INTERNAL "")
set (BUILD_CPU_DEMOS OFF CACHE INTERNAL "")
set (USE_GLUT OFF CACHE INTERNAL "")
set (BUILD_BULLET3 ON CACHE INTERNAL "")
set (BUILD_PYBULLET OFF CACHE INTERNAL "")
set (BUILD_ENET ON CACHE INTERNAL "")
set (BUILD_CLSOCKET ON CACHE INTERNAL "")
set (BUILD_EGL OFF CACHE INTERNAL "")
set (BUILD_OPENGL3_DEMOS OFF CACHE INTERNAL "")
set (BUILD_BULLET2_DEMOS OFF CACHE INTERNAL "")
set (BUILD_EXTRAS OFF CACHE INTERNAL "")
set (INSTALL_LIBS OFF CACHE INTERNAL "")
set (INSTALL_EXTRA_LIBS OFF CACHE INTERNAL "")
set (BUILD_UNIT_TESTS OFF CACHE INTERNAL "")
set (INSTALL_CMAKE_FILES OFF CACHE INTERNAL "")

add_subdirectory (bullet3)
add_library (bullet3 STATIC "bullet3/src/btBulletCollisionAll.cpp" "bullet3/src/btBulletDynamicsAll.cpp")# "bullet3/src/btLinearMathAll.cpp")
target_link_libraries(bullet3 Bullet3Collision Bullet3Dynamics LinearMath)
target_include_directories (bullet3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/bullet3/src)


if (BX_WINDOW_BACKEND STREQUAL "GLFW")
	if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set (GLFW_BUILD_WAYLAND OFF)
	endif ()

    # GLFW lib
	add_subdirectory (glfw)

	# ImGui GLFW
	add_library (imgui_glfw STATIC "imgui/backends/imgui_impl_glfw.cpp")
	target_link_libraries (imgui_glfw glfw)
	target_include_directories (imgui_glfw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

endif ()

if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
    # GLAD lib
	add_subdirectory (glad)

	# ImGui OpenGL3
	add_library (imgui_opengl3 STATIC "imgui/backends/imgui_impl_opengl3.cpp")
	target_include_directories (imgui_opengl3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

elseif (BX_GRAPHICS_BACKEND STREQUAL "Vulkan")
    # Vulkan
	find_package(Vulkan REQUIRED)
	add_library(vulkan INTERFACE)
	target_link_libraries(vulkan INTERFACE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)# Vulkan::glslang Vulkan::glslang-spirv)

	# ImGui Vulkan
	add_library (imgui_vulkan STATIC "imgui/backends/imgui_impl_vulkan.cpp")
	target_include_directories (imgui_vulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
	target_link_libraries(imgui_vulkan vulkan)
	
	# Glslang
	set (ENABLE_OPT OFF)
	#set (BUILD_EXTERNAL OFF)
	#set (BUILD_SHARED_LIBS OFF)
	add_subdirectory (glslang)

endif ()

if (BX_BUILD_EDITOR)
	# STB lib
	add_library (stb INTERFACE)
	set_property(TARGET stb APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/stb)

	# Assimp
	add_subdirectory (assimp)

	# ImPlot lib
	add_library (implot STATIC "implot/implot.cpp" "implot/implot_demo.cpp" "implot/implot_items.cpp")
	target_link_libraries (implot imgui)
	target_include_directories (implot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/implot)

	# ImGuizmo lib
	add_library (ImGuizmo STATIC "ImGuizmo/GraphEditor.cpp" "ImGuizmo/ImCurveEdit.cpp" "ImGuizmo/ImGradient.cpp" "ImGuizmo/ImGuizmo.cpp" "ImGuizmo/ImSequencer.cpp")
	target_link_libraries (ImGuizmo imgui)
	target_include_directories (ImGuizmo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)

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