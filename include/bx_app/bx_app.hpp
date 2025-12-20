#ifndef BX_APP
#define BX_APP

#include <bx_core.hpp>
#include <fmt/format.h>

#define bx_log_set_category_types(T, types) bx::log_set_category_types(bx::category_mask<bx_category_##T##_t>(), types)
#define _bx_log(T, lvl, fstr, ...) bx::logf(lvl, fstr, ##__VA_ARGS__)
#define _bx_log_v(T, lvl, fstr, ...) bx::logf_v(lvl, bx::category_mask<bx_category_##T##_t>(), __func__, __FILE__, __LINE__, fstr, ##__VA_ARGS__)
#define bx_info(T, fstr, ...) _bx_log_v(T, bx::log_t::INFO, fstr, ##__VA_ARGS__)
#define bx_warn(T, fstr, ...) _bx_log_v(T, bx::log_t::WARN, fstr, ##__VA_ARGS__)
#define bx_error(T, fstr, ...) _bx_log_v(T, bx::log_t::ERROR, fstr, ##__VA_ARGS__)
#define bx_fatal(T, fstr, ...) _bx_log_v(T, bx::log_t::FATAL, fstr, ##__VA_ARGS__)
#define bx_debug(T, fstr, ...) _bx_log_v(T, bx::log_t::DEBUG, fstr, ##__VA_ARGS__)
#define bx_verbose(T, fstr, ...) _bx_log(T, bx::log_t::VERBOSE, fstr, ##__VA_ARGS__)

#define bx_assert(expr, msg) do { if (!(expr)) { bx_fatal(bx, "Assertion failed '{}'", msg); } } while (0)
#define bx_ensure(expr) bx_assert(expr, #expr)

#define bx_profile(T) bx::profile_t _bx_concat(_bx_profile_, __LINE__){ bx::category_mask<bx_category_##T##_t>(), __func__, __func__, __FILE__, __LINE__ }
#define bx_profile_scope(T, label) bx::profile_t _bx_concat(_bx_profile_, __LINE__){ bx::category_mask<bx_category_##T##_t>(), label, __func__, __FILE__, __LINE__ }

namespace bx
{
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

	bx_api result_t app_init(const app_config_t& config) bx_noexcept;

	bx_api void app_shutdown() bx_noexcept;

	bx_api bool app_begin_frame() bx_noexcept;

	bx_api void app_end_frame(bool present, bool should_close) bx_noexcept;

	bx_api f64 app_time_seconds() bx_noexcept;

	bx_api f64 app_frame_time() bx_noexcept;

	bx_api u64 app_timestamp_ms() bx_noexcept;

	bx_api u64 app_timestamp_ns() bx_noexcept;

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

	bx_api void log_set_callback(log_callback_t cb) bx_noexcept;

	bx_api void log_set_category_types(category_t category, log_t types) bx_noexcept;

	bx_api void log(log_t level, cstring str) bx_noexcept;

	template<typename... Args>
	inline void logf(log_t level, cstring fstr, Args&&... args) bx_noexcept
	{
		auto str = fmt::format(fstr, std::forward<Args>(args)...);
		log(level, str.c_str());
	}

	bx_api void log_v(log_t level, category_t category, cstring func, cstring file, i32 line, cstring str) bx_noexcept;

	template<typename... Args>
	inline void logf_v(log_t level, category_t category, cstring func, cstring file, i32 line, cstring fstr, Args&&... args) bx_noexcept
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

	bx_api void profile_start() bx_noexcept;
	bx_api void profile_stop() bx_noexcept;
	bx_api varray<profile_entry_t> profile_get_entries() bx_noexcept;

	bx_api void profile_push(u64 category, cstring label, cstring func, cstring file, i32 line) bx_noexcept;
	bx_api void profile_pop() bx_noexcept;

	bx_api varray<cstring> profile_get_stack() bx_noexcept;

	struct bx_api profile_t
	{
		explicit profile_t(u64 category, cstring label, cstring func, cstring file, i32 line) bx_noexcept
		{
			profile_push(category, label, func, file, line);
		}
		~profile_t() bx_noexcept { profile_pop(); }
		profile_t(const profile_t&) = delete;
		profile_t& operator=(const profile_t&) = delete;
	};

	// ------------------------------------------
	// -             FileIO API                 -
	// ------------------------------------------

	using filepath_t = fstring<512>;

	bx_api bool file_add_drive(cstring drive, cstring root) bx_noexcept;

	bx_api bool file_get_path(cstring filename, filepath_t& filepath) bx_noexcept;

	bx_api vstring file_get_ext(cstring filename) bx_noexcept;

	bx_api u64 file_get_timestamp(cstring filename) bx_noexcept;

	// ------------------------------------------
	// -          Configuration API             -
	// ------------------------------------------

	using config_freefn_t = void(*)(cvptr);

	bx_api void config_set(u64 type, cstring name, cvptr data, config_freefn_t free) bx_noexcept;

	bx_api cvptr config_get(u64 type, cstring name) bx_noexcept;

	bx_api bool config_has(u64 type, cstring name) bx_noexcept;

	bx_api void config_clear() bx_noexcept;

	template<typename T>
	void config_set(cstring name, T* data, config_freefn_t free = nullptr) bx_noexcept
	{
		const u64 type = type_id<T>();
		config_set(type, name, data, free ? free : [](cvptr ptr) { delete static_cast<T*>(ptr); });
	}

	template<typename T>
	T* config_get(cstring name) bx_noexcept
	{
		const u64 type = type_id<T>();
		return static_cast<T*>(config_get(type, name));
	}

	template<typename T>
	bool config_has(cstring name) bx_noexcept
	{
		return config_has(type_id<T>(), name);
	}

	// ------------------------------------------
	// -              Device API                -
	// ------------------------------------------

	struct bx_api dvc_key_state_t
	{
		bool down{ false };
		bool pressed{ false };
		bool released{ false };
	};

	bx_api bool dvc_key_down(i32 key) bx_noexcept;

	bx_api dvc_key_state_t dvc_key(i32 key) bx_noexcept;

	struct bx_api dvc_mouse_state_t
	{
		f32 x{ 0 }, y{ 0 };
		bool buttons[8]{};
	};

	bx_api dvc_mouse_state_t dvc_mouse() bx_noexcept;

	bx_api void dvc_set_cursor_visible(bool visible) bx_noexcept;

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

	struct bx_api gfx_features_t
	{
		u32 max_texture_size{ 0 };
		bool supports_compute{ false };
		bool supports_geometry_shader{ false };
	};

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
		varray<gfx_shader_macro_t> macros{};
		cstring filepath{ nullptr };
		cstring source{ nullptr };
		varray<u8> src_bin{};
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
		varray<gfx_vertex_attribute_t> attributes{};
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
		varray<handle_id> shaders{};
		gfx_topology_t topology{ gfx_topology_t::TRIANGLES };
		gfx_vertex_input_layout_t input_layout{};
		gfx_resource_layout_t resource_layout{};
		gfx_raster_state_t raster{};
		varray<gfx_color_blend_attachment_t> color_attachments{};
	};

	struct bx_api gfx_resource_set_desc_t
	{
		cstring name{ nullptr };
		//varray<gfx_resource_binding_t> bindings{};
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
		varray<gfx_attachment_desc_t> color_attachments{};
		bool has_depth{ false };
	};

	struct bx_api gfx_framebuffer_desc_t
	{
		cstring name{ nullptr };
		varray<handle_id> color_textures{};
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

	bx_api cstring gfx_backend_name() bx_noexcept;

	bx_api const gfx_features_t& gfx_get_features() bx_noexcept;

	bx_api void gfx_push_debug_group(cstring name) bx_noexcept;

	bx_api void gfx_pop_debug_group() bx_noexcept;

	bx_api void gfx_insert_debug_marker(cstring name) bx_noexcept;

	bx_api handle_id gfx_create_shader(const gfx_shader_desc_t& desc) bx_noexcept;

	bx_api void gfx_destroy_shader(handle_id handle) bx_noexcept;

	bx_api handle_id gfx_create_buffer(const gfx_buffer_desc_t& desc) bx_noexcept;

	bx_api void gfx_destroy_buffer(handle_id handle) bx_noexcept;

	bx_api u8* gfx_map_buffer(handle_id handle, u64 offset, u64 size) bx_noexcept;

	bx_api void gfx_unmap_buffer(handle_id handle) bx_noexcept;

	bx_api void gfx_update_buffer(handle_id handle, u64 dst_offset, cvptr src, u64 size) bx_noexcept;

	bx_api handle_id gfx_create_texture(const gfx_texture_desc_t& desc) bx_noexcept;

	bx_api void gfx_destroy_texture(handle_id texture) bx_noexcept;

	bx_api void gfx_upload_texture_data(handle_id texture, const u8* data, u32 region_count, const gfx_texture_region_t* regions) bx_noexcept;

	bx_api handle_id gfx_create_framebuffer(const gfx_framebuffer_desc_t& desc) bx_noexcept;

	bx_api void gfx_destroy_framebuffer(handle_id fb) bx_noexcept;

	bx_api handle_id gfx_default_framebuffer() bx_noexcept;

	bx_api handle_id gfx_create_pipeline(const gfx_pipeline_desc_t& desc) bx_noexcept;

	bx_api void gfx_destroy_pipeline(handle_id handle) bx_noexcept;

	bx_api handle_id gfx_create_resource_set(const gfx_resource_set_desc_t& desc) bx_noexcept;

	//bx_api void gfx_update_resource_set(handle_id set_handle, u32 binding_count, const gfx_resource_binding_t* bindings) bx_noexcept;

	bx_api void gfx_destroy_resource_set(handle_id set_handle) bx_noexcept;

	bx_api void gfx_clear_rt(handle_id rt, f32 cv[4]) bx_noexcept;

	bx_api void gfx_clear_ds(handle_id ds);

	bx_api void gfx_bind_pipeline(handle_id cb, handle_id pipeline) bx_noexcept;

	bx_api void gfx_bind_vertex_buffers(handle_id cb, u32 first_binding, u32 binding_count, const handle_id* vertex_buffers, const u64* offsets) bx_noexcept;

	bx_api void gfx_bind_index_buffer(handle_id cb, handle_id index_buffer, u32 index_type) bx_noexcept;

	bx_api void gfx_bind_resource_set(handle_id cb, handle_id pipeline, handle_id set, u32 set_index) bx_noexcept;

	bx_api void gfx_draw(handle_id cb, u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) bx_noexcept;

	bx_api void gfx_draw_indexed(handle_id cb, u32 index_count, u32 instance_count = 1, u32 first_index = 0, i32 vertex_offset = 0) bx_noexcept;

	bx_api void gfx_copy_buffer(handle_id cb, handle_id src, handle_id dst, u64 src_offset, u64 dst_offset, u64 size) bx_noexcept;
	
	bx_api void gfx_dispatch(handle_id cb, u32 x, u32 y, u32 z) bx_noexcept;
	
	bx_api void gfx_submit(handle_id cb) bx_noexcept; // submit and execute or immediate execute
	
	bx_api void gfx_wait_idle() bx_noexcept;
	
	bx_api void gfx_pipeline_barrier(handle_id cb, const gfx_memory_barrier_t& barrier) bx_noexcept;
}

#endif // BX_APP