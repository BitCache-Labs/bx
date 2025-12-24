#ifndef BX_STRING
#define BX_STRING

#include <bx/core.hpp>
#include <bx/array.hpp>
#include <string>

namespace bx
{
	using string = std::string; // TODO: Temp fallback to c++ stl

	template <usize N>
	struct bx_api string_fixed {}; // TODO: Implement string fixed size

	struct bx_api string_view
	{
		using iterator = char*;
		using const_iterator = const char*;

		constexpr string_view() noexcept = default;
		constexpr string_view(cstring cstr, usize size) noexcept : cstr(cstr), size(size) {}

		template<usize N>
		constexpr string_view(const nstring<N>& fstr) noexcept : cstr(fstr), size(N) {}

		constexpr const_iterator begin() const noexcept { return cstr; }
		constexpr const_iterator end() const noexcept { return cstr + size; }

		cstring cstr{ nullptr };
		usize size{ 0 };
	};
}

#endif // BX_STRING