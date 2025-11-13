#ifndef BX_HPP
#define BX_HPP

// ------------------------------------------
// -             Basic Types                -
// ------------------------------------------

#include <cstdint>
#include <cstddef>

using uchar = unsigned char;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
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

// array
template <typename T>
using carray = const T*;

template <typename T, usize N>
using farray = T[N];

template <typename T>
struct varray
{
	constexpr varray() noexcept = default;
	constexpr varray(carray<T> data, usize size) noexcept : data(data), size(size) {}
	carray<T> data{ nullptr };
	usize size{ 0 };
	constexpr const T& operator[](usize i) const noexcept { return data[i]; }
	constexpr explicit operator bool() const noexcept { return data != nullptr && size > 0; }
};

// string
using cstring = carray<char>; // utf-8

template <usize N>
using fstring = farray<char, N>;

template <>
struct varray<char>
{
	constexpr varray() noexcept = default;
	constexpr varray(cstring cstr, usize size) noexcept : cstr(cstr), size(size) {}
	cstring cstr{ nullptr };
	usize size{ 0 };
};
using vstring = varray<char>;

// ------------------------------------------
// -                Macros                  -
// ------------------------------------------

#define bx_register_type(T) template<> inline u32 bx::type_id<T>() { return bx::register_type_id(); }

#define bx_logi(fstr, ...) bx::logf_v(bx::log_t::INFO, __func__, __FILE__, __LINE__, fstr, ##__VA_ARGS__)
#define bx_logw(fstr, ...) bx::logf_v(bx::log_t::WARN, __func__, __FILE__, __LINE__, fstr, ##__VA_ARGS__)
#define bx_loge(fstr, ...) bx::logf_v(bx::log_t::ERROR, __func__, __FILE__, __LINE__, fstr, ##__VA_ARGS__)
#define bx_logf(fstr, ...) bx::logf_v(bx::log_t::FATAL, __func__, __FILE__, __LINE__, fstr, ##__VA_ARGS__)
#define bx_logv(fstr, ...) bx::logf_v(bx::log_t::VERBOSE, __func__, __FILE__, __LINE__, fstr, ##__VA_ARGS__)
#define bx_logd(fstr, ...) bx::logf_v(bx::log_t::DEBUG, __func__, __FILE__, __LINE__, fstr, ##__VA_ARGS__)

namespace bx
{
	// ------------------------------------------
	// -               Core API                 -
	// ------------------------------------------

	using handle_id = u64;
	constexpr handle_id invalid_handle = 0;

	enum struct result_t : i32
	{
		OK               = 0,
		FAIL             = -1,
		INVALID_ARGUMENT = -2,
		OUT_OF_MEMORY    = -3,
		NOT_IMPLEMENTED  = -4,
		NOT_READY        = -5
	};

	u32 register_type_id();

	template<typename>
	u32 type_id() { return 0; }

	// ------------------------------------------
	// -              Utilities                 -
	// ------------------------------------------

	constexpr u32 bit_mask(const u32 n) noexcept { return 1u << n; }

	template<typename T, usize N>
	constexpr usize array_size(const farray<T, N>&) noexcept { return N; }

	template<typename T, usize N>
	constexpr varray<T> array_vcast(const farray<T, N>& arr) noexcept { return { arr, N }; }

	// ------------------------------------------
	// -           Application API              -
	// ------------------------------------------

	struct app_config_t
	{
		int width{ 0 };
		int height{ 0 };
		cstring title{ nullptr };
		bool vsync{ true };
	};

	result_t app_init(const app_config_t& config) noexcept;

	void app_shutdown() noexcept;

	bool app_begin_frame() noexcept;

	void app_end_frame(bool present, bool should_close) noexcept;

	f64 app_time_seconds() noexcept;

	f64 app_frame_time() noexcept;

	struct key_state_t
	{
		bool down{false};
		bool pressed{false};
		bool released{false};
	};

	bool app_key_down(i32 key) noexcept;

	key_state_t app_key(i32 key) noexcept;

	struct mouse_state_t
	{
		f32 x{0}, y{0};
		bool buttons[8]{};
	};

	mouse_state_t app_mouse() noexcept;

	void app_set_cursor_visible(bool visible) noexcept;

	// ------------------------------------------
	// -             Logging API                -
	// ------------------------------------------

	enum struct log_t : u8
	{
		INFO = 0, WARN = 1, ERROR = 2, FATAL = 3, VERBOSE = 4, DEBUG = 5
	};

	using log_callback_t = void(*)(log_t, cstring);

	void log_set_callback(log_callback_t cb) noexcept;

	void log(log_t level, cstring str);

	void log_v(log_t level, cstring func, cstring file, i32 line, cstring str);

	template<typename... Args>
	inline void logf(log_t level, cstring fstr, Args&&... args);

	template<typename... Args>
	inline void logf_v(log_t level, cstring func, cstring file, i32 line, cstring fstr, Args&&... args);

	// ------------------------------------------
	// -             FileIO API                 -
	// ------------------------------------------

	bool file_add_path_drive(cstring drive, cstring root) noexcept;

	cstring file_get_path(cstring filename) noexcept;

	cstring file_get_ext(cstring filename) noexcept;

	u64 file_get_timestamp(cstring filename) noexcept;

	// ------------------------------------------
	// -          Configuration API             -
	// ------------------------------------------

	using config_freefn_t = void(*)(cvptr);

	void config_set(u64 type, cstring name, cvptr data, config_freefn_t free) noexcept;

	cvptr config_get(u64 type, cstring name) noexcept;

	bool config_has(u64 type, cstring name) noexcept;

	void config_clear() noexcept;

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
	// -             Graphics API               -
	// ------------------------------------------

	enum struct gfx_shader_lang_t : u8 { GLSL, HLSL, SPIR_V, CUSTOM };

	enum struct gfx_shader_stage_t : u8
	{
		NONE = 0u,
		VERTEX = bit_mask(0), TESS_CONTROL = bit_mask(1), TESS_EVAL = bit_mask(2), GEOMETRY = bit_mask(3),
		FRAGMENT = bit_mask(4), COMPUTE = bit_mask(5),
		ANY = VERTEX | TESS_CONTROL | TESS_EVAL | GEOMETRY | FRAGMENT | COMPUTE,
	};

	enum struct gfx_topology_t : u8 { TRIANGLES, TRIANGLE_STRIP, LINES, LINE_STRIP, POINTS };

	enum struct gfx_attribute_format_t : u8 { FLOAT, FLOAT2, FLOAT3, FLOAT4, UINT8_4_NORM };

	enum struct gfx_resource_type_t : u8
	{
		COMBINED_IMAGE_SAMPLER, STORAGE_IMAGE, UNIFORM_BUFFER, STORAGE_BUFFER, SAMPLER, TEXTURE
	};

	enum struct gfx_buffer_usage_t : u8
	{
		VERTEX = bit_mask(0), INDEX = bit_mask(1), UNIFORM = bit_mask(2), STORAGE = bit_mask(3),
		TRANSFER_SRC = bit_mask(4), TRANSFER_DST = bit_mask(5)
	};

	enum struct gfx_memory_usage_t : u8 { GPU_ONLY, CPU_TO_GPU, GPU_TO_CPU };

	enum struct gfx_texture_format_t : u8 { R8U_NORM, RG8U_NORM, RGBA8U_NORM, RGBA16F, DEPTH24_STENCIL8 };

	enum struct gfx_texture_filter_t : u8 { NEAREST, LINEAR };

	enum struct gfx_texture_wrap_t : u8 { CLAMP_TO_EDGE, REPEAT };

	enum struct gfx_texture_type_t : u8 { TEX2D, TEX3D, TEX_CUBE };

	enum struct gfx_load_op_t : u8 { LOAD = 0, CLEAR = 1, DONT_CARE = 2 };

	enum struct gfx_store_op_t : u8 { STORE = 0, DONT_CARE = 1 };

	struct gfx_features_t
	{
		u32 max_texture_size{ 0 };
		bool supports_compute{ false };
		bool supports_geometry_shader{ false };
	};

	struct gfx_shader_macro_t
	{
		cstring name{ nullptr };
		cstring value{ nullptr };
	};

	struct gfx_shader_desc_t
	{
		cstring name{ nullptr };
		gfx_shader_lang_t lang{};
		gfx_shader_stage_t stage{ gfx_shader_stage_t::NONE };
		
		varray<gfx_shader_macro_t> macros{};

		cstring filepath{ nullptr };
		cstring source{ nullptr };

		varray<u8> src_bin{};
	};

	struct gfx_pipeline_shader_t
	{
		handle_id handle{};
		cstring entry_point{ "main" };
	};

	struct gfx_vertex_attribute_t
	{
		u8 location{ 0 };
		gfx_attribute_format_t format{};
		u16 offset{ 0 };
		u32 binding{ 0 };
	};

	struct gfx_vertex_binding_t
	{
		u32 binding{ 0 };
		u32 stride{ 0 };
		u32 input_rate_per_vertex_or_instance{ 0 };
	};

	struct gfx_vertex_input_desc_t
	{
		cstring name{ nullptr };
		varray<gfx_vertex_binding_t> bindings{};
		varray<gfx_vertex_attribute_t> attributes{};
	};

	struct gfx_raster_state_t
	{
		bool cull_enable{ true };
		bool depth_test{ true };
		bool depth_write{ true };
	};

	struct gfx_color_blend_attachment_t
	{
		bool blend_enable{ false };
	};

	struct gfx_resource_binding_t
	{
		u32 binding{ 0 };
		gfx_resource_type_t type{};
		handle_id resource{ 0 };
	};

	struct gfx_resource_set_layout_desc_t
	{
		cstring name{ nullptr };
		varray<gfx_resource_binding_t> bindings{};
	};

	struct gfx_pipeline_layout_desc_t
	{
		cstring name{ nullptr };
		varray<handle_id> set_layouts{};
	};

	struct gfx_pipeline_desc_t
	{
		cstring name{ nullptr };
		varray<gfx_pipeline_shader_t> shaders{};
		gfx_topology_t topology{ gfx_topology_t::TRIANGLES };

		gfx_vertex_input_desc_t vertex_input{};

		gfx_raster_state_t raster{};
		varray<gfx_color_blend_attachment_t> color_attachments{};
		handle_id layout{};
	};

	struct gfx_buffer_desc_t
	{
		cstring name{ nullptr };
		gfx_buffer_usage_t usage{};
		gfx_memory_usage_t memory_usage{};
		u64 size{ 0 };
		const u8* data{ nullptr };
	};

	struct gfx_texture_desc_t
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

	struct gfx_texture_region_t
	{
		u8 mip_level{ 0 };
		i32 x{ 0 };
		i32 y{ 0 };
		u32 width{ 0 };
		u32 height{ 0 };
		u32 row_stride{ 0 };
	};

	struct gfx_resource_set_desc_t
	{
		cstring name{ nullptr };
		varray<gfx_resource_binding_t> bindings{};
	};

	struct gfx_attachment_desc_t
	{
		cstring name{ nullptr };
		gfx_texture_format_t format{};
		gfx_load_op_t load{ gfx_load_op_t::CLEAR };
		gfx_store_op_t store{ gfx_store_op_t::STORE };
	};

	struct gfx_renderpass_desc_t
	{
		cstring name{ nullptr };
		varray<gfx_attachment_desc_t> color_attachments{};
		bool has_depth{ false };
	};

	struct gfx_framebuffer_desc_t
	{
		cstring name{ nullptr };
		varray<handle_id> color_textures{};
		handle_id depth_texture{};
		u32 width{ 0 };
		u32 height{ 0 };
		handle_id render_pass{};
	};

	struct gfx_clear_value_t
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

	struct gfx_memory_barrier_t
	{
		gfx_shader_access_t src_access{};
		gfx_shader_access_t dst_access{};
		gfx_shader_stage_t src_stage{};
		gfx_shader_stage_t dst_stage{};
	};
	
	cstring gfx_backend_name() noexcept;

	const gfx_features_t& gfx_get_features() noexcept;

	void gfx_push_debug_group(cstring name) noexcept;

	void gfx_pop_debug_group() noexcept;

	void gfx_insert_debug_marker(cstring name) noexcept;

	handle_id gfx_create_shader(const gfx_shader_desc_t& desc) noexcept;

	void gfx_destroy_shader(handle_id handle) noexcept;

	handle_id gfx_create_resource_set_layout(const gfx_resource_set_layout_desc_t& desc) noexcept;

	void gfx_destroy_resource_set_layout(handle_id handle) noexcept;

	handle_id gfx_create_pipeline_layout(const gfx_pipeline_layout_desc_t& desc) noexcept;

	void gfx_destroy_pipeline_layout(handle_id handle) noexcept;

	handle_id gfx_create_pipeline(const gfx_pipeline_desc_t& desc) noexcept;

	void gfx_destroy_pipeline(handle_id handle) noexcept;

	handle_id gfx_create_buffer(const gfx_buffer_desc_t& desc) noexcept;

	void gfx_destroy_buffer(handle_id handle) noexcept;

	u8* gfx_map_buffer(handle_id handle, u64 offset, u64 size) noexcept;

	void gfx_unmap_buffer(handle_id handle) noexcept;

	void gfx_update_buffer(handle_id handle, u64 dst_offset, cvptr src, u64 size) noexcept;

	handle_id gfx_create_texture(const gfx_texture_desc_t& desc) noexcept;

	void gfx_destroy_texture(handle_id texture) noexcept;

	void gfx_upload_texture_data(handle_id texture, const u8* data, u32 region_count, const gfx_texture_region_t* regions) noexcept;
	
	handle_id gfx_create_resource_set(const gfx_resource_set_desc_t& desc) noexcept;

	void gfx_update_resource_set(handle_id set_handle, u32 binding_count, const gfx_resource_binding_t* bindings) noexcept;

	void gfx_destroy_resource_set(handle_id set_handle) noexcept;

	handle_id gfx_create_renderpass(const gfx_renderpass_desc_t& desc) noexcept;

	void gfx_destroy_renderpass(handle_id rp) noexcept;

	handle_id gfx_create_framebuffer(const gfx_framebuffer_desc_t& desc) noexcept;

	void gfx_destroy_framebuffer(handle_id fb) noexcept;

	handle_id gfx_default_framebuffer() noexcept;

	handle_id gfx_immediate_command_buffer() noexcept;

	void gfx_cmd_begin_renderpass(handle_id framebuffer, const gfx_clear_value_t* clear_values, u32 clear_count) noexcept;

	void gfx_cmd_end_render_pass() noexcept;

	handle_id gfx_create_command_buffer() noexcept;

	void gfx_destroy_command_buffer(handle_id cb) noexcept;

	void gfx_cmd_begin(handle_id cb) noexcept;

	void gfx_cmd_end(handle_id cb) noexcept;

	void gfx_cmd_bind_pipeline(handle_id cb, handle_id pipeline) noexcept;

	void gfx_cmd_bind_vertex_buffers(handle_id cmd, u32 first_binding, u32 binding_count, const handle_id* vertex_buffers, const u64* offsets) noexcept;

	void gfx_cmd_bind_index_buffer(handle_id cb, handle_id index_buffer, u32 index_type) noexcept;

	void gfx_cmd_bind_resource_set(handle_id cb, handle_id pipeline, handle_id set, u32 set_index) noexcept;

	void gfx_cmd_draw(handle_id cb, u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) noexcept;

	void gfx_cmd_draw_indexed(handle_id cb, u32 index_count, u32 instance_count = 1, u32 first_index = 0, i32 vertex_offset = 0) noexcept;

	void gfx_cmd_copy_buffer(handle_id cb, handle_id src, handle_id dst, u64 src_offset, u64 dst_offset, u64 size) noexcept;

	void gfx_cmd_dispatch(handle_id cb, u32 x, u32 y, u32 z) noexcept;

	void gfx_submit(handle_id cb) noexcept; // submit and execute or immediate execute

	void gfx_wait_idle() noexcept;

	void gfx_pipeline_barrier(handle_id cb, const gfx_memory_barrier_t& barrier) noexcept;
}

#endif //BX_HPP