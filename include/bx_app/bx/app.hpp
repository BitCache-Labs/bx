#ifndef BX_APP
#define BX_APP

#include <bx/core.hpp>
#include <bx/type.hpp>
#include <bx/array.hpp>
#include <bx/string.hpp>

#include <fmt/format.h>

#define bx_register_category(T) \
	struct bx_api bx_category_##T##_t {}; \
	template<> inline bx::category_t bx::category_mask<bx_category_##T##_t>() noexcept { static const auto c = bx::register_category(#T); return c; }

#define bx_log_set_category_types(T, types) bx::log_set_category_types(bx::category_mask<bx_category_##T##_t>(), types)
#define _bx_log(T, lvl, fstr, ...) bx::logf(lvl, fstr, ##__VA_ARGS__)
#define _bx_log_v(T, lvl, fstr, ...) bx::logf_v(lvl, bx::category_mask<bx_category_##T##_t>(), bx_func, bx_file, bx_line, fstr, ##__VA_ARGS__)
#define bx_info(T, fstr, ...) _bx_log_v(T, bx::log_t::INFO, fstr, ##__VA_ARGS__)
#define bx_warn(T, fstr, ...) _bx_log_v(T, bx::log_t::WARN, fstr, ##__VA_ARGS__)
#define bx_error(T, fstr, ...) _bx_log_v(T, bx::log_t::ERROR, fstr, ##__VA_ARGS__)
#define bx_fatal(T, fstr, ...) _bx_log_v(T, bx::log_t::FATAL, fstr, ##__VA_ARGS__)
#define bx_debug(T, fstr, ...) _bx_log_v(T, bx::log_t::DEBUG, fstr, ##__VA_ARGS__)
#define bx_verbose(T, fstr, ...) _bx_log(T, bx::log_t::VERBOSE, fstr, ##__VA_ARGS__)

#define bx_assert(expr, msg) do { if (!(expr)) { bx_fatal(bx, "Assertion failed '{}'", msg); } } while (0)
#define bx_ensure(expr) bx_assert(expr, #expr)

#define bx_profile(T) bx::profile_t _bx_concat(_bx_profile_, bx_line){ bx::category_mask<bx_category_##T##_t>(), bx_func, bx_func, bx_file, bx_line }
#define bx_profile_scope(T, label) bx::profile_t _bx_concat(_bx_profile_, bx_line){ bx::category_mask<bx_category_##T##_t>(), label, bx_func, bx_file, bx_line }

namespace bx
{
	// TODO: Lets rethink how categories are done, atm not very clean design
	bx_api category_t register_category(cstring name) noexcept;

	template<typename>
	category_t category_mask() noexcept { return 0; }

	bx_api cstring category_name(category_t id) noexcept;

	template<typename T>
	cstring category_name() noexcept { return category_name(category_mask<T>()); }

	bx_api array_view<category_t> get_categories() noexcept;

	// ------------------------------------------
	// -           Application API              -
	// ------------------------------------------

	struct bx_api app_config_t
	{
		i32 width{ 0 };
		i32 height{ 0 };
		cstring title{ nullptr };
		bool vsync{ true };
	};

	bx_api result_t app_init(const app_config_t& config) noexcept;

	bx_api void app_shutdown() noexcept;

	bx_api bool app_begin_frame() noexcept;

	bx_api void app_end_frame(bool present, bool should_close) noexcept;

	bx_api f64 app_time_seconds() noexcept;

	bx_api f64 app_frame_time() noexcept;

	bx_api u64 app_timestamp_ms() noexcept;

	bx_api u64 app_timestamp_ns() noexcept;

	// ------------------------------------------
	// -             Logging API                -
	// ------------------------------------------

	enum struct log_t : u8
	{
		INFO = bit_mask(0),
		WARN = bit_mask(1),
		ERROR = bit_mask(2),
		FATAL = bit_mask(3),
		VERBOSE = bit_mask(4),
		DEBUG = bit_mask(5)
	};

	using log_callback_t = void(*)(log_t, category_t, cstring, cstring, i32, cstring);

	bx_api void log_set_callback(log_callback_t cb) noexcept;

	bx_api void log_set_category_types(category_t category, log_t types) noexcept;

	bx_api void log(log_t level, cstring str) noexcept;

	template<typename... Args>
	inline void logf(log_t level, cstring fstr, Args&&... args) noexcept
	{
		auto str = fmt::format(fstr, std::forward<Args>(args)...);
		log(level, str.c_str());
	}

	bx_api void log_v(log_t level, category_t category, cstring func, cstring file, i32 line, cstring str) noexcept;

	template<typename... Args>
	inline void logf_v(log_t level, category_t category, cstring func, cstring file, i32 line, cstring fstr, Args&&... args) noexcept
	{
		auto str = fmt::format(fstr, std::forward<Args>(args)...);
		log_v(level, category, func, file, line, str.c_str());
	}

	// ------------------------------------------
	// -            Profiling API               -
	// ------------------------------------------

	struct bx_api profile_entry_t
	{
		u64 category{ 0 };
		u32 depth{ 0 };
		u64 start_ts{ 0 };
		u64 end_ts{ 0 };
		cstring label{ nullptr };
		cstring file{ nullptr };
		i32 line{ 0 };
	};

	bx_api void profile_start() noexcept;
	bx_api void profile_stop() noexcept;
	bx_api array_view<profile_entry_t> profile_get_entries() noexcept;

	bx_api void profile_push(u64 category, cstring label, cstring func, cstring file, i32 line) noexcept;
	bx_api void profile_pop() noexcept;

	bx_api array_view<cstring> profile_get_stack() noexcept;

	struct bx_api profile_t
	{
		explicit profile_t(u64 category, cstring label, cstring func, cstring file, i32 line) noexcept
		{
			profile_push(category, label, func, file, line);
		}
		~profile_t() noexcept { profile_pop(); }
		profile_t(const profile_t&) = delete;
		profile_t& operator=(const profile_t&) = delete;
	};

	// ------------------------------------------
	// -             FileIO API                 -
	// ------------------------------------------

	bx_api bool file_add_drive(cstring drive, cstring root) noexcept;

	bx_api string file_get_path(cstring filename) noexcept;

	bx_api string_view file_get_ext(cstring filename) noexcept;

	bx_api u64 file_get_timestamp(cstring filename) noexcept;

	// ------------------------------------------
	// -          Configuration API             -
	// ------------------------------------------

	using config_freefn_t = void(*)(cvptr);

	bx_api void config_set(u64 type, cstring name, cvptr data, config_freefn_t free) noexcept;

	bx_api cvptr config_get(u64 type, cstring name) noexcept;

	bx_api bool config_has(u64 type, cstring name) noexcept;

	bx_api void config_clear() noexcept;

	template<typename T>
	void config_set(cstring name, T* data, config_freefn_t free = nullptr) noexcept
	{
		const u64 type = type_id<T>();
		config_set(type, name, data, free ? free : [](cvptr ptr) { delete static_cast<T*>(ptr); });
	}

	template<typename T>
	T* config_get(cstring name) noexcept
	{
		const u64 type = type_id<T>();
		return static_cast<T*>(config_get(type, name));
	}

	template<typename T>
	bool config_has(cstring name) noexcept
	{
		return config_has(type_id<T>(), name);
	}

	// ------------------------------------------
	// -              Device API                -
	// ------------------------------------------

	bx_api void dvc_screen_size(i32* w, i32* h) noexcept;

	struct bx_api dvc_key_state_t
	{
		bool down{ false };
		bool pressed{ false };
		bool released{ false };
	};

	bx_api bool dvc_key_down(i32 key) noexcept;

	bx_api dvc_key_state_t dvc_key(i32 key) noexcept;

	struct bx_api dvc_mouse_state_t
	{
		f32 x{ 0 }, y{ 0 };
		bool buttons[8]{};
	};

	bx_api dvc_mouse_state_t dvc_mouse() noexcept;

	bx_api void dvc_set_cursor_visible(bool visible) noexcept;

	// ------------------------------------------
	// -             Graphics API               -
	// ------------------------------------------

	enum struct gfx_shader_lang_t : u8 { GLSL, HLSL, SPIR_V, CUSTOM };

	enum struct gfx_shader_stage_t : u8
	{
		NONE = 0u,
		VERTEX, TESS_CONTROL, TESS_EVAL, GEOMETRY, FRAGMENT, COMPUTE
	};

	enum struct gfx_topology_t : u8 { TRIANGLES, TRIANGLE_STRIP, LINES, LINE_STRIP, POINTS };

	enum struct gfx_attribute_format_t : u8 { FLOAT32 };

	enum struct gfx_resource_type_t : u8
	{
		COMBINED_IMAGE_SAMPLER, STORAGE_IMAGE, UNIFORM_BUFFER, STORAGE_BUFFER, SAMPLER, TEXTURE
	};

	enum struct gfx_buffer_usage_t : u8 { VERTEX, INDEX, UNIFORM, STORAGE, INDIRECT };

	enum struct gfx_memory_usage_t : u8 { GPU_ONLY, CPU_TO_GPU, GPU_TO_CPU };

	enum struct gfx_texture_format_t : u8 { R8U_NORM, RG8U_NORM, RGBA8U_NORM, RGBA16F, DEPTH24_STENCIL8 };

	enum struct gfx_texture_filter_t : u8 { NEAREST, LINEAR };

	enum struct gfx_texture_wrap_t : u8 { CLAMP_TO_EDGE, REPEAT };

	enum struct gfx_texture_type_t : u8 { TEX2D, TEX3D, TEX_CUBE };

	enum struct gfx_load_op_t : u8 { LOAD = 0, CLEAR = 1, DONT_CARE = 2 };

	enum struct gfx_store_op_t : u8 { STORE = 0, DONT_CARE = 1 };

	struct bx_api gfx_shader_macro_t
	{
		cstring name{ nullptr };
		cstring value{ nullptr };
	};

	struct bx_api gfx_shader_desc_t
	{
		cstring name{ nullptr };
		gfx_shader_stage_t stage{ gfx_shader_stage_t::NONE };
		cstring entrypoint{ nullptr };
		gfx_shader_lang_t lang{};
		array_view<gfx_shader_macro_t> macros{};
		cstring filepath{ nullptr };
		cstring source{ nullptr };
		array_view<u8> src_bin{};
	};

	struct bx_api gfx_buffer_desc_t
	{
		cstring name{ nullptr };
		gfx_buffer_usage_t usage{};
		gfx_memory_usage_t memory_usage{};
		u64 size{ 0 };
		const u8* data{ nullptr };
	};

	struct bx_api gfx_texture_desc_t
	{
		cstring name{ nullptr };
		gfx_texture_type_t type{ gfx_texture_type_t::TEX2D };
		gfx_texture_format_t format{};
		u32 width{ 0 };
		u32 height{ 0 };
		u32 depth{ 1 };
		u8 mip_levels{ 1 };
		gfx_texture_filter_t min_filter{ gfx_texture_filter_t::NEAREST };
		gfx_texture_filter_t mag_filter{ gfx_texture_filter_t::NEAREST };
		gfx_texture_wrap_t wrap_u{ gfx_texture_wrap_t::CLAMP_TO_EDGE };
		gfx_texture_wrap_t wrap_v{ gfx_texture_wrap_t::CLAMP_TO_EDGE };
		gfx_texture_wrap_t wrap_w{ gfx_texture_wrap_t::CLAMP_TO_EDGE };
	};

	struct bx_api gfx_texture_region_t
	{
		u8 mip_level{ 0 };
		i32 x{ 0 };
		i32 y{ 0 };
		u32 width{ 0 };
		u32 height{ 0 };
		u32 row_stride{ 0 };
	};

	struct bx_api gfx_vertex_attribute_t
	{
		u8 location{ 0 };
		u8 binding{ 0 };
		u8 count{ 0 };
		gfx_attribute_format_t format{};
		bool normalized{ false };
		u32 offset{ 0 };
		u8 input_rate_per_vertex_or_instance{ 0 };
	};

	struct bx_api gfx_vertex_input_layout_t
	{
		array_view<gfx_vertex_attribute_t> attributes{};
	};

	struct bx_api gfx_raster_state_t
	{
		bool cull_enable{ true };
		bool depth_test{ true };
		bool depth_write{ true };
	};

	struct bx_api gfx_color_blend_attachment_t
	{
		bool blend_enable{ false };
	};

	struct bx_api gfx_resource_layout_t
	{
	};

	struct bx_api gfx_pipeline_desc_t
	{
		cstring name{ nullptr };
		array_view<handle_id> shaders{};
		gfx_topology_t topology{ gfx_topology_t::TRIANGLES };
		gfx_vertex_input_layout_t input_layout{};
		gfx_resource_layout_t resource_layout{};
		gfx_raster_state_t raster{};
		array_view<gfx_color_blend_attachment_t> color_attachments{};
	};

	struct bx_api gfx_resource_set_desc_t
	{
		cstring name{ nullptr };
		//array_view<gfx_resource_binding_t> bindings{};
	};

	struct bx_api gfx_attachment_desc_t
	{
		cstring name{ nullptr };
		gfx_texture_format_t format{};
		gfx_load_op_t load{ gfx_load_op_t::CLEAR };
		gfx_store_op_t store{ gfx_store_op_t::STORE };
	};

	struct bx_api gfx_renderpass_desc_t
	{
		cstring name{ nullptr };
		array_view<gfx_attachment_desc_t> color_attachments{};
		bool has_depth{ false };
	};

	struct bx_api gfx_framebuffer_desc_t
	{
		cstring name{ nullptr };
		array_view<handle_id> color_textures{};
		handle_id depth_texture{};
		u32 width{ 0 };
		u32 height{ 0 };
		handle_id render_pass{};
	};

	struct bx_api gfx_clear_value_t
	{
		f32 r{ 0 }, g{ 0 }, b{ 0 }, a{ 0 };
		f32 depth{ 0 };
		u32 stencil{ 0 };
	};

	enum struct gfx_shader_access_t : u32
	{
		NONE = 0u,
		READ = bit_mask(0),
		WRITE = bit_mask(1),
		TRANSFER = bit_mask(2)
	};

	struct bx_api gfx_memory_barrier_t
	{
		gfx_shader_access_t src_access{};
		gfx_shader_access_t dst_access{};
		gfx_shader_stage_t src_stage{};
		gfx_shader_stage_t dst_stage{};
	};

	struct bx_api gfx_info_t
	{
		cstring backend{ nullptr };
		cstring device{ nullptr };
		cstring adapter{ nullptr };
		cstring api_version{ nullptr };
		cstring shader_version{ nullptr };

		struct features_t
		{
			u32 max_texture_size{ 0 };
			bool supports_compute{ false };
			bool supports_geometry_shader{ false };
		};
		features_t features{};
	};

	bx_api const gfx_info_t& gfx_get_info() noexcept;

	bx_api void gfx_push_debug_group(cstring name) noexcept;

	bx_api void gfx_pop_debug_group() noexcept;

	bx_api void gfx_insert_debug_marker(cstring name) noexcept;

	bx_api handle_id gfx_create_shader(const gfx_shader_desc_t& desc) noexcept;

	bx_api void gfx_destroy_shader(handle_id handle) noexcept;

	bx_api handle_id gfx_create_buffer(const gfx_buffer_desc_t& desc) noexcept;

	bx_api void gfx_destroy_buffer(handle_id handle) noexcept;

	bx_api u8* gfx_map_buffer(handle_id handle, u64 offset, u64 size) noexcept;

	bx_api void gfx_unmap_buffer(handle_id handle) noexcept;

	bx_api void gfx_update_buffer(handle_id handle, u64 dst_offset, cvptr src, u64 size) noexcept;

	bx_api handle_id gfx_create_texture(const gfx_texture_desc_t& desc) noexcept;

	bx_api void gfx_destroy_texture(handle_id texture) noexcept;

	bx_api void gfx_upload_texture_data(handle_id texture, const u8* data, u32 region_count, const gfx_texture_region_t* regions) noexcept;

	bx_api handle_id gfx_create_framebuffer(const gfx_framebuffer_desc_t& desc) noexcept;

	bx_api void gfx_destroy_framebuffer(handle_id fb) noexcept;

	bx_api handle_id gfx_default_framebuffer() noexcept;

	bx_api handle_id gfx_create_pipeline(const gfx_pipeline_desc_t& desc) noexcept;

	bx_api void gfx_destroy_pipeline(handle_id handle) noexcept;

	bx_api handle_id gfx_create_resource_set(const gfx_resource_set_desc_t& desc) noexcept;

	//bx_api void gfx_update_resource_set(handle_id set_handle, u32 binding_count, const gfx_resource_binding_t* bindings) noexcept;

	bx_api void gfx_destroy_resource_set(handle_id set_handle) noexcept;

	bx_api void gfx_clear_rt(handle_id rt, f32 cv[4]) noexcept;

	bx_api void gfx_clear_ds(handle_id ds);

	bx_api void gfx_bind_pipeline(handle_id cb, handle_id pipeline) noexcept;

	bx_api void gfx_bind_vertex_buffers(handle_id cb, u32 first_binding, u32 binding_count, const handle_id* vertex_buffers, const u64* offsets) noexcept;

	bx_api void gfx_bind_index_buffer(handle_id cb, handle_id index_buffer, u32 index_type) noexcept;

	bx_api void gfx_bind_resource_set(handle_id cb, handle_id pipeline, handle_id set, u32 set_index) noexcept;

	bx_api void gfx_draw(handle_id cb, u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) noexcept;

	bx_api void gfx_draw_indexed(handle_id cb, u32 index_count, u32 instance_count = 1, u32 first_index = 0, i32 vertex_offset = 0) noexcept;

	bx_api void gfx_copy_buffer(handle_id cb, handle_id src, handle_id dst, u64 src_offset, u64 dst_offset, u64 size) noexcept;
	
	bx_api void gfx_dispatch(handle_id cb, u32 x, u32 y, u32 z) noexcept;
	
	bx_api void gfx_submit(handle_id cb) noexcept; // submit and execute or immediate execute
	
	bx_api void gfx_wait_idle() noexcept;
	
	bx_api void gfx_pipeline_barrier(handle_id cb, const gfx_memory_barrier_t& barrier) noexcept;
}

bx_register_category(bx)

#endif // BX_APP