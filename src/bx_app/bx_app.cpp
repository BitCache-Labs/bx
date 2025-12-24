#include <bx_app_impl.hpp>

#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <unordered_map>

static bx::array<bx::category_t> g_categories{};
static std::unordered_map<bx::category_t, cstring> g_categories_map{};

static bx::log_callback_t g_log_callback = nullptr;
static std::unordered_map<bx::category_t, bx::log_t> g_log_masks{};

static bool g_profiling = false;
static u32 g_profile_depth = 0;
static std::vector<cstring> g_profile_stack{};
static std::vector<bx::profile_entry_t> g_profile_entries{};

static std::unordered_map<u64, cstring> g_drives{};

struct config_data_t
{
	cvptr data{ nullptr };
	bx::config_freefn_t free{ nullptr };
};

static std::unordered_map<u64, config_data_t> g_config{};

bx::category_t bx::register_category(cstring name) noexcept
{
	static category_t g_id{ 0 };
	category_t next = bit_mask(g_id++);
	g_categories.emplace_back(next);
	g_categories_map.insert(std::make_pair(next, name));
	return next;
}

cstring bx::category_name(category_t id) noexcept
{
	auto it = g_categories_map.find(id);
	if (it == g_categories_map.end())
		return "unknown";
	return it->second;
}

bx::array_view<bx::category_t> bx::get_categories() noexcept
{
	return { g_categories };
}

bx::result_t bx::app_init(const app_config_t& config) noexcept
{
	bx_profile(bx);

	if (!bx::dvc_init(config))
		return result_t::FAIL;

	if (!bx::gfx_init(config))
		return result_t::FAIL;

	return result_t::OK;
}

void bx::app_shutdown() noexcept
{
	bx_profile(bx);

	bx::dvc_shutdown();
	bx::gfx_shutdown();
}

u64 bx::app_timestamp_ms() noexcept
{
	bx_profile(bx);

	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	return static_cast<u64>(ms.count());
}

u64 bx::app_timestamp_ns() noexcept
{
	bx_profile(bx);

	auto now = std::chrono::high_resolution_clock::now();
	auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
	return static_cast<u64>(ns.count());
}

static u64 hash_cstring(cstring str)
{
	bx_profile(bx);

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

void bx::log_set_callback(const log_callback_t cb) noexcept
{
	bx_profile(bx);

	g_log_callback = cb;
}

void bx::log_set_category_types(category_t category, log_t types) noexcept
{
	bx_profile(bx);

	g_log_masks[category] = types;
}

static cstring get_func_name(cstring func)
{
	bx_profile(bx);

	return func;
}

static cstring get_file_name(cstring file)
{
	bx_profile(bx);

	cstring last_slash = std::strrchr(file, '/');
	if (!last_slash) last_slash = std::strrchr(file, '\\');
	return last_slash ? last_slash + 1 : file;
}

static void crash(cstring reason, cstring file, i32 line)
{
	std::cerr << "\n";
	std::cerr << "====================[ FATAL ERROR ]====================\n";
	if (reason)
		std::cerr << "Reason: " << reason << "\n";
	if (file && line)
		std::cerr << "Location: " << file << ":" << line << "\n";
	std::cerr << "The application has encountered an unexpected state and must terminate.\n";
	std::cerr << "Please report this issue with the following stack trace:\n\n";

	if (!g_profile_stack.empty())
	{
		for (usize i = g_profile_stack.size(); i-- > 0; )
		{
			std::cerr << i << ": " << g_profile_stack[i] << "()\n";
		}
	}
	else
	{
		std::cerr << "(stack trace unavailable)\n";
	}

	std::abort();
}

void bx::log(log_t level, cstring str) noexcept
{
	switch (level)
	{
	case log_t::INFO:
	case log_t::WARN:
	case log_t::VERBOSE:
	case log_t::DEBUG:
		std::cout << str << std::endl;
		break;

	case log_t::ERROR:
		std::cerr << str << std::endl;
		break;

	case log_t::FATAL:
		std::cerr << str << std::endl;
		crash(nullptr, nullptr, 0);
		break;
	}
}

void bx::log_v(log_t level, category_t category, cstring func, cstring file, i32 line, cstring msg) noexcept
{
	auto it = g_log_masks.find(category);
	if (it != g_log_masks.end() && ((u32)it->second & (u32)level) == 0)
		return;

	cstring level_str = nullptr;
	switch (level) {
	case log_t::INFO:		level_str = "INFO"; break;
	case log_t::WARN:		level_str = "WARN"; break;
	case log_t::ERROR:		level_str = "ERROR"; break;
	case log_t::FATAL:		level_str = "FATAL"; break;
	case log_t::VERBOSE:	level_str = "VERBOSE"; break;
	case log_t::DEBUG:		level_str = "DEBUG"; break;
	}

	cstring func_name = get_func_name(func);
	cstring file_name = get_file_name(file);

	cstring category_str = category_name(category);
	std::string formatted = fmt::format("[{}:{}] ({}:{}) {}: {}", level_str, category_str, file_name, line, func_name, msg);
	
	if (g_log_callback)
		g_log_callback(level, category, func_name, file_name, line, formatted.c_str());

	switch (level)
	{
	case log_t::INFO:
	case log_t::WARN:
	case log_t::VERBOSE:
	case log_t::DEBUG:
		std::cout << formatted << std::endl;
		break;

	case log_t::ERROR:
		std::cerr << formatted << std::endl;
		break;

	case log_t::FATAL:
		std::cerr << formatted << std::endl;
		crash(msg, file_name, line);
		break;
	}
}

void bx::profile_start() noexcept
{
	bx_profile(bx);

	g_profile_entries.clear();
	g_profiling = true;
}

void bx::profile_stop() noexcept
{
	bx_profile(bx);

	g_profiling = false;
}

bx::array_view<bx::profile_entry_t> bx::profile_get_entries() noexcept
{
	bx_profile(bx);

	return { g_profile_entries.data(), g_profile_entries.size() };
}

void bx::profile_push(u64 category, cstring label, cstring func, cstring file, i32 line) noexcept
{
	const auto depth = g_profile_depth++;
	g_profile_stack.emplace_back(func);

	if (!g_profiling)
		return;

	profile_entry_t entry{};
	entry.category = category;
	entry.depth = depth;
	entry.start_ts = app_timestamp_ns();
	entry.end_ts = 0;
	entry.label = label;
	entry.file = file;
	entry.line = line;

	g_profile_entries.emplace_back(entry);
}

void bx::profile_pop() noexcept
{
	if (g_profile_depth > 0)
		g_profile_depth--;

	if (!g_profile_stack.empty())
		g_profile_stack.pop_back();

	if (!g_profiling)
		return;

	if (!g_profile_entries.empty())
		g_profile_entries.back().end_ts = app_timestamp_ns();
}

bx_api bx::array_view<cstring> bx::profile_get_stack() noexcept
{
	bx_profile(bx);

	return { g_profile_stack.data(), g_profile_stack.size() };
}

bool bx::file_add_drive(cstring drive, cstring root) noexcept
{
	bx_profile(bx);

	u64 hash = hash_cstring(drive);
	if (g_drives.find(hash) != g_drives.end())
		return false;
	g_drives.insert(std::make_pair(hash, root));
	return true;
}

bx::string bx::file_get_path(cstring filename) noexcept
{
	bx_profile(bx);

	if (!filename || filename[0] != '[')
		return "";

	cstring end = std::strchr(filename, ']');
	if (!end)
		return "";

	usize drive_len = 1 + end - filename;
	nstring<512> drive_name{};
	std::memcpy(drive_name, filename, drive_len);
	drive_name[drive_len] = '\0';

	u64 hash = hash_cstring(drive_name);
	auto it = g_drives.find(hash);
	if (it == g_drives.end())
		return "";

	nstring<512> filepath{};
	filepath[0] = '\0';
	std::strncpy(filepath, it->second, 511);
	filepath[511] = '\0';

	usize root_len = std::strlen(filepath);
	if (root_len > 0 && filepath[root_len - 1] != '/' && filepath[root_len - 1] != '\\')
	{
		if (root_len < 511)
		{
			filepath[root_len] = '/';
			filepath[root_len + 1] = '\0';
			++root_len;
		}
	}

	cstring relative = end + 1;
	if (*relative == '/' || *relative == '\\')
		++relative;

	usize remaining_space = 511 - root_len;
	std::strncat(filepath, relative, remaining_space);
	filepath[511] = '\0';

	return filepath;
}

bx::string_view bx::file_get_ext(cstring filename) noexcept
{
	bx_profile(bx);

	if (!filename)
		return bx::string_view{ nullptr, 0 };

	cstring dot = nullptr;
	for (cstring p = filename; *p; ++p)
	{
		if (*p == '.')
			dot = p;
		else if (*p == '/' || *p == '\\')
			dot = nullptr;
	}

	if (!dot || *(dot + 1) == '\0')
		return bx::string_view{ nullptr, 0 };

	return bx::string_view{ dot + 1, std::strlen(dot + 1) };
}

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

u64 bx::file_get_timestamp(cstring filename) noexcept
{
	bx_profile(bx);

	string fp = bx::file_get_path(filename);
	if (fp.empty())
		return 0;

#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA info;
	if (!GetFileAttributesExA(fp.c_str(), GetFileExInfoStandard, &info))
		return 0;

	// Convert FILETIME (100ns intervals since Jan 1, 1601) to milliseconds since Unix epoch
	u64 high = static_cast<u64>(info.ftLastWriteTime.dwHighDateTime);
	u64 low = static_cast<u64>(info.ftLastWriteTime.dwLowDateTime);
	u64 timestamp_100ns = (high << 32) | low;

	constexpr u64 EPOCH_DIFF_MS = 11644473600000ULL; // 1970 - 1601 in ms
	return timestamp_100ns / 10000 - EPOCH_DIFF_MS;

#elif defined(__APPLE__)
	return 0;

#else
	struct stat st;
	if (stat(fp.c_str(), &st) != 0)
		return 0;
	return static_cast<u64>(st.st_mtim.tv_sec) * 1000 + st.st_mtim.tv_nsec / 1000000;
#endif
}

void bx::config_set(const u64 type, cstring name, cvptr data, const config_freefn_t free) noexcept
{
	bx_profile(bx);

	const u64 hash = hash_cstring(name) ^ type;
	config_data_t cfg{};
	cfg.data = data;
	cfg.free = free;
	g_config[hash] = cfg;
}

cvptr bx::config_get(const u64 type, cstring name) noexcept
{
	bx_profile(bx);

	const u64 hash = hash_cstring(name) ^ type;
	const auto it = g_config.find(hash);
	if (it == g_config.end())
		return nullptr;
	return it->second.data;
}

bool bx::config_has(const u64 type, cstring name) noexcept
{
	bx_profile(bx);

	const u64 hash = hash_cstring(name) ^ type;
	return g_config.find(hash) != g_config.end();
}

void bx::config_clear() noexcept
{
	bx_profile(bx);

	for (const auto& cfg : g_config)
	{
		if (cfg.second.free)
			cfg.second.free(cfg.second.data);
	}
	g_config.clear();
}