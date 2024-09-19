#ifndef LANGUAGE_H
#define LANGUAGE_H

#define MAX_BINDINGS_PER_SET 32

#ifdef OPENGL

#define BINDING(_set, _binding) binding = _binding

#define gl_VertexIndex gl_VertexID

// TODO: double check opengl impl
vec2 uvToClip(vec2 uv)
{
	return (uv - 0.5.xx) * vec2(2.0, -2.0);
}

vec2 clipToUv(vec2 clip)
{
	return clip * vec2(0.5, -0.5) + 0.5;
}

#elif defined(VULKAN)

#define BINDING(_set, _binding) set = _set, binding = _binding

vec2 uvToClip(vec2 uv)
{
	return (uv - 0.5.xx) * vec2(2.0, -2.0);
}

vec2 clipToUv(vec2 clip)
{
	return clip * vec2(0.5, -0.5) + 0.5;
}

#endif // VULKAN

#endif // LANGUAGE_H