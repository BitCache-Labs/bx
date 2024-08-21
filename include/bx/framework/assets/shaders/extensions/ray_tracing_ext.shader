#ifndef RAY_TRACING_EXT_H
#define RAY_TRACING_EXT_H

#ifdef OPENGL

// ERROR
"Ray Tracing is not supported on OpenGL."

#elif defined(VULKAN)

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require

#endif // VULKAN

#endif // RAY_TRACING_EXT_H