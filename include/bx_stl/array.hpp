#ifndef BX_ARRAY
#define BX_ARRAY

#include <bx_core.hpp>
#include <vector>

namespace bx
{
	template <typename T>
	using array = std::vector<T>; // TODO: Temp fallback to c++ stl
}

#endif // BX_ARRAY