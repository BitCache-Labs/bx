#ifndef BX_FIXED_ARRAY
#define BX_FIXED_ARRAY

#include <bx_core.hpp>
#include <array>

namespace bx
{
	template <typename T, usize N>
	using fixed_array = std::array<T, N>; // TODO: Temp fallback to c++ stl
}

#endif // BX_FIXED_ARRAY