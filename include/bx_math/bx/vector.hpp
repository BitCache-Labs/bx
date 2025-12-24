#ifndef BX_VECTOR
#define BX_VECTOR

#include <bx/core.hpp>

namespace bx
{
	struct vec2
	{
		vec2() : x(0), y(0) {};
		vec2(f32 x, f32 y)
			: x(x), y(y)
		{};

		union
		{
			struct
			{
				f32 x, y;
			};

			f32 data[2];
		};
	};

	struct vec3
	{
		vec3() : x(0), y(0), z(0) {};
		vec3(f32 x, f32 y, f32 z)
			: x(x), y(y), z(z)
		{};

		union
		{
			struct
			{
				f32 x, y, z;
			};

			f32 data[3];
		};
	};

	struct vec4
	{
		vec4() : x(0), y(0), z(0), w(0) {};
		vec4(f32 x, f32 y, f32 z, f32 w)
			: x(x), y(y), z(z), w(w)
		{};

		union
		{
			struct
			{
				f32 x, y, z, w;
			};

			f32 data[4];
		};
	};
}

#endif // BX_VECTOR