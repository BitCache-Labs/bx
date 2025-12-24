#ifndef BX_CORE_HPP
#define BX_CORE_HPP

// ------------------------------------------
// -             Basic Types                -
// ------------------------------------------

#include <cstdint>
#include <cstddef>

using uchar = unsigned char;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

using vptr = void*;
using cvptr = const void*;
using uptr = uintptr_t;
using usize = size_t;
using isize = ptrdiff_t;

template <typename T>
using carray = const T*;

template <typename T, usize N>
using narray = T[N];

using cstring = const char*;

template <usize N>
using nstring = char[N];

// ------------------------------------------
// -                Macros                  -
// ------------------------------------------

#define bx_api

#if defined(_MSC_VER)
#define bx_force_inline __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define bx_force_inline inline __attribute__((always_inline))
#else
#define bx_force_inline inline
#endif

#define bx_nodiscard
//#define bx_nodiscard [[nodiscard]]
#define bx_maybe_unused
//#define bx_maybe_unused [[maybe_unused]]
#define bx_deprecated
//#define bx_deprecated [[deprecated]]

#define bx_file __FILE__
#define bx_line __LINE__
#define bx_func __func__

#if defined(__GNUC__) || defined(__clang__)
#define bx_pretty_func __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define bx_pretty_func __FUNCSIG__
#else
#define bx_pretty_func bx_func
#endif

#define _bx_expand_impl(x) #x
#define _bx_expand(x) _bx_expand_impl(x)
#define _bx_concat_impl(x, y) x##y
#define _bx_concat(x, y) _bx_concat_impl(x, y)

namespace bx
{
	// ------------------------------------------
	// -               Core API                 -
	// ------------------------------------------

	using handle_id = u64;
	constexpr handle_id invalid_handle = 0;

	using type_t = u64;
	constexpr type_t invalid_type = 0;

	using category_t = u64;
	constexpr category_t default_category = 0;

	enum struct result_t : i32
	{
		OK = 0,
		FAIL = -1,
		INVALID_ARGUMENT = -2,
		OUT_OF_MEMORY = -3,
		NOT_IMPLEMENTED = -4,
		NOT_READY = -5,
		UNKNOWN = -6
	};

	// ------------------------------------------
	// -              Utilities                 -
	// ------------------------------------------

	constexpr u32 bit_mask(const u32 n) noexcept { return 1u << n; }

	template<typename T, usize N>
	constexpr usize array_size(const narray<T, N>&) noexcept { return N; }
}

#endif //BX_CORE_HPP