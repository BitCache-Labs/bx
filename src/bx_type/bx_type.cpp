#include <bx/type.hpp>

#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <unordered_map>

static std::vector<bx::type_t> g_types{};
static std::unordered_map<bx::type_t, cstring> g_types_map{};

bx_register_type(u8)
bx_register_type(u16)
bx_register_type(u32)
bx_register_type(u64)
bx_register_type(i8)
bx_register_type(i16)
bx_register_type(i32)
bx_register_type(i64)
bx_register_type(f32)
bx_register_type(f64)
bx_register_type(cstring)

bx::type_t bx::register_type(cstring name) bx_noexcept
{
	static type_t g_id{ 0 };
	type_t next = g_id++;
	g_types.emplace_back(next);
	g_types_map.insert(std::make_pair(next, name));
	return next;
}

cstring bx::type_name(type_t id) bx_noexcept
{
	auto it = g_types_map.find(id);
	if (it == g_types_map.end())
		return "unknown";
	return it->second;
}

bx::array_view<bx::type_t> bx::get_types() bx_noexcept
{
	bx::array_view<type_t> view{};
	view.data = g_types.data();
	view.size = g_types.size();
	return view;
}