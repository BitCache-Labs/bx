#ifndef COLOR_HELPERS_H
#define COLOR_HELPERS_H

float linearToLuma(vec3 linear)
{
	return dot(linear, vec3(0.2126, 0.7152, 0.0722));
}

#endif // COLOR_HELPERS_H