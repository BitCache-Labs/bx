#ifndef BX_HASH_MAP
#define BX_HASH_MAP

#include <bx/core.hpp>
#include <unordered_map>

namespace bx
{
	template <typename K, typename V>
	using hash_map = std::unordered_map<K, V>; // TODO: Temp fallback to c++ stl
}

#endif // BX_HASH_MAP