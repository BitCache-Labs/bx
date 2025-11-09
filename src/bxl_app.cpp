#include "bxl.hpp"

#include <iostream>
#include <unordered_map>

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

namespace bx
{
	static log_callback_t g_log_callback = nullptr;

	struct config_data
	{
		void* data{nullptr};
		free_fn free{nullptr};
	};

	static std::unordered_map<u64, config_data> g_config{};
}

static u64 bx_hash_cstring(cstring str)
{
	// FNV-1a 64-bit
	if (!str)
		return 0;

	u64 hash = 14695981039346656037ull; // FNV_OFFSET_BASIS
	while (*str)
	{
		hash ^= static_cast<uchar>(*str++);
		hash *= 1099511628211ull; // FNV_PRIME
	}
	return hash;
}

void bx::app_config_set(const u64 type, cstring name, void* data, const free_fn free) noexcept
{
	const u64 hash = bx_hash_cstring(name) ^ type;
	config_data cfg{};
	cfg.data       = data;
	cfg.free       = free;
	g_config[hash] = cfg;
}

void* bx::app_config_get(const u64 type, cstring name) noexcept
{
	const u64 hash = bx_hash_cstring(name) ^ type;
	const auto it  = g_config.find(hash);
	if (it == g_config.end())
		return nullptr;
	return it->second.data;
}

bool bx::app_config_has(const u64 type, cstring name) noexcept
{
	const u64 hash = bx_hash_cstring(name) ^ type;
	return g_config.find(hash) != g_config.end();
}

void bx::app_config_clear() noexcept
{
	for (const auto& cfg : g_config)
	{
		if (cfg.second.free)
			cfg.second.free(cfg.second.data);
	}
	g_config.clear();
}

// -----------------------------------------------------------------------------
// Logging
// -----------------------------------------------------------------------------
void bx::log_set_callback(const log_callback_t cb) noexcept
{
	g_log_callback = cb;
}

void bx::log(cstring msg)
{
	if (g_log_callback)
		g_log_callback(msg);
	else
		std::cout << msg << std::endl;
}
