# Platform specific libs
#if (BX_PLATFORM_BACKEND STREQUAL "Common")
#	add_subdirectory (bx-platform-common)
#endif ()

if (BX_WINDOW_BACKEND STREQUAL "GLFW")
	add_custom_target(build_all_backends ALL DEPENDS bx-platform-glfw)
	add_subdirectory (bx-platform-glfw)
endif ()

#if (BX_GRAPHICS_BACKEND STREQUAL "OpenGL")
#    add_subdirectory (bx-platform-opengl)
#endif ()

#if (BX_AUDIO_BACKEND STREQUAL "PortAudio")
#    add_subdirectory (bx-platform-portaudio)
#endif ()