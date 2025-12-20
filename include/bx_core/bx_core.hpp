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

// ------------------------------------------
// -                Macros                  -
// ------------------------------------------

#define bx_api
#define bx_noexcept noexcept

#define _bx_expand_impl(x) #x
#define _bx_expand(x) _bx_expand_impl(x)
#define _bx_concat_impl(x, y) x##y
#define _bx_concat(x, y) _bx_concat_impl(x, y)

#define bx_register_type(T) template<> inline bx::type_t bx::type_id<T>() bx_noexcept { static const auto t = bx::register_type(#T); return t; }
#define bx_register_category(T) \
	struct bx_api bx_category_##T##_t {}; \
	template<> inline bx::category_t bx::category_mask<bx_category_##T##_t>() bx_noexcept { static const auto c = bx::register_category(#T); return c; }

// ------------------------------------------
// -              Containers                -
// ------------------------------------------

// array
template <typename T>
using carray = const T*;

template <typename T, usize N>
using farray = T[N];

template <typename T>
struct bx_api varray
{
	using iterator = T*;
	using const_iterator = const T*;
	constexpr varray() bx_noexcept = default;
	constexpr varray(carray<T> data, usize size) bx_noexcept : data(data), size(size) {}
	constexpr const T& operator[](usize i) const bx_noexcept { return data[i]; }
	constexpr explicit operator bool() const bx_noexcept { return data != nullptr && size > 0; }
	constexpr const_iterator begin() const bx_noexcept { return data; }
	constexpr const_iterator end() const bx_noexcept { return data + size; }
	carray<T> data{ nullptr };
	usize size{ 0 };
};

// string
using cstring = carray<char>; // utf-8

template <usize N>
using fstring = farray<char, N>;

template <>
struct bx_api varray<char>
{
	using iterator = char*;
	using const_iterator = const char*;
	constexpr varray() bx_noexcept = default;
	constexpr varray(cstring cstr, usize size) bx_noexcept : cstr(cstr), size(size) {}
	constexpr const_iterator begin() const bx_noexcept { return cstr; }
	constexpr const_iterator end() const bx_noexcept { return cstr + size; }
	cstring cstr{ nullptr };
	usize size{ 0 };
};
using vstring = varray<char>;

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

	bx_api type_t register_type(cstring name) bx_noexcept;

	template<typename>
	type_t type_id() bx_noexcept { return 0; }

	bx_api cstring type_name(type_t id) bx_noexcept;

	template<typename T>
	cstring type_name() bx_noexcept { return type_name(type_id<T>()); }

	template<typename T>
	cstring type_name(T*) bx_noexcept { return type_name(type_id<T>()); }

	bx_api varray<type_t> get_types() bx_noexcept;

	// TODO: Lets rethink how categories are done, atm not very clean design
	bx_api category_t register_category(cstring name) bx_noexcept;

	template<typename>
	category_t category_mask() bx_noexcept { return 0; }

	bx_api cstring category_name(category_t id) bx_noexcept;

	template<typename T>
	cstring category_name() bx_noexcept { return category_name(category_mask<T>()); }

	bx_api varray<category_t> get_categories() bx_noexcept;

	// ------------------------------------------
	// -              Utilities                 -
	// ------------------------------------------

	constexpr u32 bit_mask(const u32 n) bx_noexcept { return 1u << n; }

	template<typename T, usize N>
	constexpr usize array_size(const farray<T, N>&) bx_noexcept { return N; }

	template<typename T, usize N>
	constexpr varray<T> array_vcast(const farray<T, N>& arr) bx_noexcept { return { arr, N }; }
}

bx_register_category(bx)

#endif //BX_CORE_HPP