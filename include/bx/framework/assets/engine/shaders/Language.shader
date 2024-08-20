#ifndef LANGUAGE_H
#define LANGUAGE_H

#define MAX_BINDINGS_PER_SET 32

#ifdef OPENGL

#define BINDING(_set, _binding) binding = _binding

#define gl_VertexIndex gl_VertexID

#elif defined(VULKAN)

#define BINDING(_set, _binding) set = _set, binding = _binding

#endif // VULKAN

#endif // LANGUAGE_H