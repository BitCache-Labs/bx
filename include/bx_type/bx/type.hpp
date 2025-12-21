#ifndef BX_TYPE
#define BX_TYPE

#include <bx/core.hpp>
#include <bx/array.hpp>

#define bx_register_type(T) template<> inline bx::type_t bx::type_id<T>() bx_noexcept { static const auto t = bx::register_type(#T); return t; }

namespace bx
{
	bx_api type_t register_type(cstring name) bx_noexcept;

	template<typename>
	type_t type_id() bx_noexcept { return 0; }

	bx_api cstring type_name(type_t id) bx_noexcept;

	template<typename T>
	cstring type_name() bx_noexcept { return type_name(type_id<T>()); }

	template<typename T>
	cstring type_name(T*) bx_noexcept { return type_name(type_id<T>()); }

	bx_api array_view<type_t> get_types() bx_noexcept;
}

#endif // BX_TYPE