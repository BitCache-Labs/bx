#ifndef BX_ARRAY
#define BX_ARRAY

#include <bx/core.hpp>
#include <vector>
#include <array>

namespace bx
{
	template <typename T>
	using array = std::vector<T>; // TODO: Temp fallback to c++ stl

	template <typename T, usize N>
	using array_fixed = std::array<T, N>; // TODO: Temp fallback to c++ stl

	template <typename T>
	struct bx_api array_view
	{
		using iterator = T*;
		using const_iterator = const T*;
		
		constexpr array_view() bx_noexcept = default;
		constexpr array_view(carray<T> data, usize size) bx_noexcept : data(data), size(size) {}

		template<usize N>
		constexpr array_view(const narray<T, N>& arr) bx_noexcept : data(arr), size(N) {}
		
		constexpr const T& operator[](usize i) const bx_noexcept { return data[i]; }
		constexpr explicit operator bool() const bx_noexcept { return data != nullptr && size > 0; }
		
		constexpr const_iterator begin() const bx_noexcept { return data; }
		constexpr const_iterator end() const bx_noexcept { return data + size; }
		
		carray<T> data{ nullptr };
		usize size{ 0 };
	};
}

#endif // BX_ARRAY