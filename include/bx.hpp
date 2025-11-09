#ifndef BX_HPP
#define BX_HPP

#include <cstdint>

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

#define cstring const char*

// Type reflection
inline u64 bx_register_type_id()
{
	static u64 g_id = 0;
	return g_id++;
}

template<typename>
u64 bx_type_id() { return 0; }

#define bx_register_type(T) template<> u64 bx_type_id<T>() { return bx_register_type_id(); }

// Utilities
template<typename T, unsigned N>
constexpr unsigned bx_array_size(const T (&)[N]) noexcept { return N; }

constexpr u32 bx_bit(const u32 n) noexcept { return 1u << (n - 1); }

namespace bx
{
	using handle_id = u64;

	// Generic result codes - functions that might fail can return this in future
	enum struct result_t : i32
	{
		OK               = 0,
		FAIL             = -1,
		INVALID_ARGUMENT = -2,
		OUT_OF_MEMORY    = -3,
		NOT_IMPLEMENTED  = -4,
		NOT_READY        = -5
	};

	// ------------------------------------------
	// -           Application API              -
	// ------------------------------------------

	/* Description:
	 *   Initialize the application/windowing layer (GLFW in your current backend).
	 * Implementation notes:
	 *   - OpenGL (GLFW): create window/context, initialize loader (GLAD), set callbacks,
	 *     setup ImGui if used, initialize timing.
	 *   - Vulkan/D3D12/Metal: create instance/device/surface, create swap-chain, command queues.
	 */
	result_t app_init() noexcept;

	/* Description:
	 *   Clean up and shut down the windowing and graphics subsystems.
	 * Implementation notes:
	 *   - GL: destroy window/context, shutdown ImGui, terminate GLFW.
	 *   - Vulkan/D3D12/Metal: wait idle, destroy swap-chain, free device/resources.
	 */
	void app_shutdown() noexcept;

	/* Description:
	 *   Poll events and begin a new frame. Returns false when application/window should close.
	 * Implementation notes:
	 *   - GL: glfwPollEvents(), ImGui new frame calls, update timing and one-frame input state.
	 *   - Vulkan/DX12/Metal: acquire next swap-chain image and prepare command buffer for recording.
	 */
	bool app_begin_frame() noexcept;

	/* Description:
	 *   End the current frame and optionally present. 'should_close' can request window close.
	 * Implementation notes:
	 *   - GL: render ImGui, swap buffers (glfwSwapBuffers) if present==true.
	 *   - Vulkan/DX12/Metal: submit command buffer, present swap-chain image, handle semaphores/fences.
	 */
	void app_end_frame(bool present, bool should_close) noexcept;

	using free_fn = void(*)(void*);

	void app_config_set(u64 type, cstring name, void* data, free_fn free) noexcept;

	void* app_config_get(u64 type, cstring name) noexcept;

	bool app_config_has(u64 type, cstring name) noexcept;

	void app_config_clear() noexcept;

	template<typename T>
	void app_config_set(cstring name, T* data, free_fn free = nullptr) noexcept
	{
		const u64 type = bx_type_id<T>();
		app_config_set(type, name, data, free ? free : [](void* ptr) { delete static_cast<T*>(ptr); });
	}

	template<typename T>
	T* app_config_get(cstring name) noexcept
	{
		const u64 type = bx_type_id<T>();
		return static_cast<T*>(app_config_get(type, name));
	}

	template<typename T>
	bool app_config_has(cstring name) noexcept
	{
		return app_config_has(bx_type_id<T>(), name);
	}

	// App timing & input
	/* Description:
	 *   Return elapsed seconds since app initialization.
	 * Implementation notes:
	 *   - GL: return glfwGetTime() (or a monotonic clock).
	 *   - Vulkan/DX12/Metal: same, use a high precision clock on host.
	 */
	f64 app_time_seconds() noexcept;

	/* Description:
	 *   Return the delta time (seconds) for the last frame.
	 * Implementation notes:
	 *   - GL: compute delta in app_begin_frame using current and previous timestamps.
	 *   - Other backends: same approach, keep last-frame timestamp on host.
	 */
	f64 app_frame_time() noexcept;

	struct key_state
	{
		bool down{false};
		bool pressed{false};
		bool released{false};
	};

	/* Description:
	 *   Query whether a key is currently down.
	 * Implementation notes:
	 *   - GL: query cached GLFW key state array updated by callbacks or glfwGetKey.
	 *   - Vulkan/DX12/Metal (windowing): same host-side key state via windowing library.
	 */
	bool app_key_down(i32 key) noexcept;

	/* Description:
	 *   Retrieve per-frame key state (down, pressed this frame, released this frame).
	 * Implementation notes:
	 *   - GL: maintain arrays for down/pressed/released toggled by GLFW key callback and reset pressed/released each frame.
	 *   - Other backends: same concept using host input events.
	 */
	key_state app_key(i32 key) noexcept;

	struct mouse_state
	{
		f32 x{0}, y{0};
		bool buttons[8]{};
	};

	/* Description:
	 *   Get mouse position (window coords) and button states.
	 * Implementation notes:
	 *   - GL: use glfwSetCursorPos callback to update cached mouse coords and glfwSetMouseButton callback for buttons.
	 *   - Other backends: update via the platform/windowing events.
	 */
	mouse_state app_mouse() noexcept;

	/* Description:
	 *   Set whether the OS cursor is visible (and whether pointer is captured).
	 * Implementation notes:
	 *   - GL: glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL/GLFW_CURSOR_DISABLED).
	 *   - Vulkan/DX12/Metal: forward to platform windowing system.
	 */
	void app_set_cursor_visible(bool visible) noexcept;

	// ------------------------------------------
	// -             Logging API                -
	// ------------------------------------------

	// Optional logging callback
	using log_callback_t = void(*)(cstring msg);

	/* Description:
	 *   Provide an optional callback for engine log messages.
	 * Implementation notes:
	 *   - Store function pointer; use it for error/warning/info messages instead of std::cout.
	 *   - Backend should never assume callback is present.
	 */
	void log_set_callback(log_callback_t cb) noexcept;

	void log(cstring msg);

	// ------------------------------------------
	// -              FileIO API                -
	// ------------------------------------------

	void file_add_path_drive(cstring drive, cstring root) noexcept;

	// ------------------------------------------
	// -             Graphics API               -
	// ------------------------------------------

	struct gfx_features
	{
		u32 max_texture_size{0};
		bool supports_compute{false};
		bool supports_geometry_shader{false};
	};

	const gfx_features& gfx_get_features() noexcept;

	// ---------- utilities ----------
	/* Description:
	 *   Return a null-terminated string identifying the active backend implementation (e.g., "opengl", "vulkan").
	 * Implementation notes:
	 *   - Backend should return a constant or cached string identifying itself.
	 */
	cstring gfx_backend_name() noexcept;

	// profiling / timestamp helpers can be added later

	// ---------- opaque debug helpers ----------
	/* Description:
	 *   Attach a human-readable debug name to an opaque handle for tooling.
	 * Implementation notes:
	 *   - GL: if glObjectLabel is available, call glObjectLabel with the right GL enum and object id (backend must map handle->GL id/type).
	 *   - Vulkan: call vkSetDebugUtilsObjectNameEXT.
	 *   - D3D12/Metal: use respective label APIs (SetName / setLabel).
	 */
	void gfx_set_debug_name(handle_id object, cstring name) noexcept;

	/* Description:
	 *   Push a GPU debug group for hierarchical debug markers.
	 * Implementation notes:
	 *   - GL: glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION,...).
	 *   - Vulkan: cmdBeginDebugUtilsLabelEXT on command buffer.
	 *   - D3D12/Metal: use PIX markers or push/pop region APIs.
	 */
	void gfx_push_debug_group(cstring name) noexcept;

	/* Description:
	 *   Pop the previously pushed GPU debug group.
	 * Implementation notes:
	 *   - GL: glPopDebugGroup().
	 *   - Vulkan: cmdEndDebugUtilsLabelEXT.
	 *   - D3D12/Metal: use appropriate pop API for labels/markers.
	 */
	void gfx_pop_debug_group() noexcept;

	/* Description:
	 *   Insert an instantaneous GPU debug marker.
	 * Implementation notes:
	 *   - GL: glDebugMessageInsert with GL_DEBUG_TYPE_MARKER.
	 *   - Vulkan: cmdInsertDebugUtilsLabelEXT.
	 *   - D3D12/Metal: platform-specific marker APIs.
	 */
	void gfx_insert_debug_marker(cstring name) noexcept;

	// ---------- shader / pipeline ----------
	enum struct gfx_shader_lang : u8 { GLSL, HLSL, SPIR_V, CUSTOM };

	// flags for shader stages
	enum struct gfx_shader_stage : u8
	{
		NONE     = 0u,
		VERTEX   = bx_bit(1), TESS_CONTROL = bx_bit(2), TESS_EVAL = bx_bit(3), GEOMETRY = bx_bit(4),
		FRAGMENT = bx_bit(5), COMPUTE      = bx_bit(6),
		ANY      = VERTEX | TESS_CONTROL | TESS_EVAL | GEOMETRY | FRAGMENT | COMPUTE,
	};

	struct gfx_shader_desc
	{
		gfx_shader_lang lang{};
		gfx_shader_stage stage{gfx_shader_stage::NONE};

		cstring filepath{nullptr};
		cstring source{nullptr};

		const u8* src_bin{nullptr};
		u64 src_bin_size{0};
	};

	/* Description:
	 *   Create a shader module or object from source, file, or binary blob.
	 * Implementation notes:
	 *   - GL: compile GLSL into a GL shader object; for SPIR-V use GL_ARB_gl_spirv + glSpecialize if available.
	 *   - Vulkan: create VkShaderModule from SPIR-V; HLSL must be cross-compiled to SPIR-V or DXIL depending on backend.
	 *   - D3D12/Metal: accept precompiled bytecode or compile/translate HLSL/MSL offline.
	 */
	handle_id gfx_create_shader(const gfx_shader_desc& desc) noexcept;

	/* Description:
	 *   Destroy a previously created shader handle and free resources.
	 * Implementation notes:
	 *   - GL: glDeleteShader on the GL shader id.
	 *   - Vulkan: vkDestroyShaderModule.
	 *   - Other: free/Release native shader objects.
	 */
	void gfx_destroy_shader(handle_id handle) noexcept;

	// Pipeline-level shader entry
	struct gfx_pipeline_shader
	{
		handle_id handle{};
		cstring entry_point{"main"};
	};

	// Small enum for primitive/topology
	enum struct gfx_topology : u8 { TRIANGLES, TRIANGLE_STRIP, LINES, LINE_STRIP, POINTS };

	// Vertex attribute formats
	enum struct gfx_attribute_format : u8 { FLOAT, FLOAT2, FLOAT3, FLOAT4, UINT8_4_NORM };

	struct gfx_vertex_attribute
	{
		u8 location{0};
		gfx_attribute_format format{};
		u16 offset{0};
		u32 binding{0};
	};

	struct gfx_vertex_binding
	{
		u32 binding{0};
		u32 stride{0};
		u32 input_rate_per_vertex_or_instance{0};
	};

	struct gfx_vertex_input_desc
	{
		const gfx_vertex_binding* bindings{nullptr};
		u32 binding_count{0};
		const gfx_vertex_attribute* attributes{nullptr};
		u32 attribute_count{0};
	};

	// Blend/depth/stencil/rasterizer can be extended; this is a minimal family
	struct gfx_raster_state
	{
		bool cull_enable = true;
		bool depth_test  = true;
		bool depth_write = true;
	};

	struct gfx_color_blend_attachment
	{
		bool blend_enable = false;
	};

	// pipeline layout / resource bindings
	enum struct gfx_resource_type : u8
	{
		COMBINED_IMAGE_SAMPLER, STORAGE_IMAGE, UNIFORM_BUFFER, STORAGE_BUFFER, SAMPLER, TEXTURE
	};

	struct gfx_resource_binding
	{
		u32 binding{0};
		gfx_resource_type type{};
		handle_id resource{0};
	};

	struct gfx_resource_set_layout_desc
	{
		const gfx_resource_binding* bindings{nullptr};
		u32 binding_count{0};
	};

	/* Description:
	 *   Create a resource-set layout (descriptor layout) that describes bindings and types.
	 * Implementation notes:
	 *   - Vulkan/D3D12: create a descriptor set layout / root signature entry.
	 *   - GL: store CPU-side metadata used later to validate bindings; no native GL object necessary.
	 */
	handle_id gfx_create_resource_set_layout(const gfx_resource_set_layout_desc& desc) noexcept;

	/* Description:
	 *   Destroy a previously created resource set layout handle.
	 * Implementation notes:
	 *   - Vulkan/D3D12: destroy native descriptor set layout/root signature as needed.
	 *   - GL: remove CPU-side metadata.
	 */
	void gfx_destroy_resource_set_layout(handle_id handle) noexcept;

	struct gfx_pipeline_layout_desc
	{
		const handle_id* set_layouts{nullptr};
		u32 set_layout_count{0};
	};

	/* Description:
	 *   Create a pipeline layout that groups resource-set layouts and push-constant ranges.
	 * Implementation notes:
	 *   - Vulkan/D3D12: pipeline layout maps directly to VkPipelineLayout / root signature.
	 *   - GL: store CPU-side layout metadata (used when validating resource set bindings for program).
		*   - This layout is referenced by pipelines to know expected binding sets.
	 */
	handle_id gfx_create_pipeline_layout(const gfx_pipeline_layout_desc& desc) noexcept;

	/* Description:
	 *   Destroy a pipeline layout handle and free-associated metadata.
	 * Implementation notes:
	 *   - Vulkan/D3D12: destroy the native pipeline layout if owned.
	 *   - GL: remove stored metadata.
	 */
	void gfx_destroy_pipeline_layout(handle_id handle) noexcept;

	struct gfx_pipeline_desc
	{
		const gfx_pipeline_shader* shaders{nullptr};
		u8 shader_count{0};
		gfx_topology topology{gfx_topology::TRIANGLES};

		// NOTE: vertex_input is a part of the pipeline (modern approach).
		// For GL backends implementers should create a VAO internally here and store it inside the pipeline object.
		gfx_vertex_input_desc vertex_input{};

		gfx_raster_state raster{};
		const gfx_color_blend_attachment* color_attachments{nullptr};
		u32 color_attachment_count{0};
		handle_id layout{}; // pipeline layout handle
	};

	/* Description:
	 *   Create a graphics pipeline object which encapsulates shaders and fixed-function state,
	 *   including vertex input layout (modern approach: pipeline owns vertex layout).
	 * Implementation notes:
	 *   - Vulkan/D3D12/Metal: create an immutable pipeline/PSO using supplied vertex_input, raster and blend state and pipeline layout.
	 *   - GL: create/link a GL program from provided shaders, **create and configure a VAO internally** using vertex_input,
	 *     store the GL program id + VAO handle inside the returned pipeline handle. Vertex buffers are still bound at draw-time,
	 *     but attribute format/locations and bindings are set up here.
	 */
	handle_id gfx_create_pipeline(const gfx_pipeline_desc& desc) noexcept;

	/* Description:
	 *   Destroy a pipeline object and free native resources.
	 * Implementation notes:
	 *   - Vulkan/D3D12/Metal: destroy pipeline object.
	 *   - GL: glDeleteProgram and glDeleteVertexArrays for the stored VAO if created internally.
	 */
	void gfx_destroy_pipeline(handle_id handle) noexcept;

	// ---------- buffers ----------
	enum struct gfx_buffer_usage : u8
	{
		VERTEX       = bx_bit(1), INDEX = bx_bit(2), UNIFORM = bx_bit(3), STORAGE = bx_bit(4), TRANSFER_SRC = bx_bit(5),
		TRANSFER_DST = bx_bit(6)
	};

	enum struct gfx_memory_usage : u8 { GPU_ONLY, CPU_TO_GPU, GPU_TO_CPU };

	struct gfx_buffer_desc
	{
		gfx_buffer_usage usage{};
		gfx_memory_usage memory_usage{};
		u64 size{0};
		const u8* data{nullptr};
	};

	/* Description:
	 *   Create a buffer resource (vertex/index/uniform/storage).
	 * Implementation notes:
	 *   - Vulkan/D3D12/Metal: create GPU buffer with appropriate memory properties and possible staging strategies.
	 *   - GL: create GL buffer (glGenBuffers/glBufferData) and allocate usage according to memory_usage (GL_STATIC_DRAW/GL_DYNAMIC_DRAW).
	 */
	handle_id gfx_create_buffer(const gfx_buffer_desc& desc) noexcept;

	/* Description:
	 *   Destroy a buffer and free-associated memory.
	 * Implementation notes:
	 *   - GL: glDeleteBuffers.
	 *   - Vulkan/D3D12/Metal: free GPU resource and underlying memory.
	 */
	void gfx_destroy_buffer(handle_id handle) noexcept;

	/* Description:
	 *   Map a portion of a buffer to CPU address space for writable/readable access.
	 * Implementation notes:
	 *   - Vulkan: vkMapMemory with offset/size and proper memory type.
	 *   - GL: glMapBufferRange on the underlying buffer binding.
	 *   - D3D12/Metal: either use an upload heap or a staging resource; mapping semantics differ per API.
	 */
	u8* gfx_map_buffer(handle_id handle, u64 offset, u64 size) noexcept;

	/* Description:
	 *   Unmap previously mapped buffer.
	 * Implementation notes:
	 *   - Vulkan: vkUnmapMemory.
	 *   - GL: glUnmapBuffer.
	 *   - Ensure proper memory barriers/synchronization after unmap for GPU visibility.
	 */
	void gfx_unmap_buffer(handle_id handle) noexcept;

	/* Description:
	 *   Update buffer content without explicit mapping (backend may use staging copy).
	 * Implementation notes:
	 *   - Vulkan/D3D12: perform staging buffer + command copy or use persistent mapped memory.
	 *   - GL: glBufferSubData or glMapBufferRange depending on memory_usage.
	 */
	void gfx_update_buffer(handle_id handle, u64 dst_offset, const void* src, u64 size) noexcept;

	// ---------- textures ----------
	enum struct gfx_texture_format : u8 { R8U_NORM, RG8U_NORM, RGBA8U_NORM, RGBA16F, DEPTH24_STENCIL8 };

	enum struct gfx_texture_filter : u8 { NEAREST, LINEAR };

	enum struct gfx_texture_wrap : u8 { CLAMP_TO_EDGE, REPEAT };

	enum struct gfx_texture_type : u8 { TEX2D, TEX3D, TEX_CUBE };

	struct gfx_texture_desc
	{
		gfx_texture_type type{gfx_texture_type::TEX2D};
		gfx_texture_format format{};
		u32 width{0};
		u32 height{0};
		u32 depth{1};
		u8 mip_levels{1};
		gfx_texture_filter min_filter{gfx_texture_filter::NEAREST};
		gfx_texture_filter mag_filter{gfx_texture_filter::NEAREST};
		gfx_texture_wrap wrap_u{gfx_texture_wrap::CLAMP_TO_EDGE};
		gfx_texture_wrap wrap_v{gfx_texture_wrap::CLAMP_TO_EDGE};
		gfx_texture_wrap wrap_w{gfx_texture_wrap::CLAMP_TO_EDGE};
	};

	/* Description:
	 *   Create a texture resource with the given format, dimensions and sampler parameters.
	 * Implementation notes:
	 *   - GL: glGenTextures/glTexStorage2D and set sampler/filter/wrap; allocate storage and optionally upload levels.
	 *   - Vulkan/D3D12/Metal: create image resource, allocate memory, create image view and sampler object as needed.
	 */
	handle_id gfx_create_texture(const gfx_texture_desc& desc) noexcept;

	/* Description:
	 *   Destroy texture and associated views/sampler if owned.
	 * Implementation notes:
	 *   - GL: glDeleteTextures and delete sampler if separate.
	 *   - Vulkan/D3D12/Metal: destroy image view and free memory.
	 */
	void gfx_destroy_texture(handle_id texture) noexcept;

	struct gfx_texture_region
	{
		u8 mip_level{0};
		i32 x{0};
		i32 y{0};
		u32 width{0};
		u32 height{0};
		u32 row_stride{0}; // bytes per row in source data (0 => tightly packed)
	};

	/* Description:
	 *   Upload pixels to texture regions (one or several sub-rectangles / mip levels).
	 * Implementation notes:
	 *   - GL: bind texture, call glTexSubImage2D or glCompressedTexSubImage2D; handle row alignment with glPixelStore.
	 *   - Vulkan/D3D12/Metal: use staging buffer + copy command to image and transition image layouts appropriately.
	 */
	void gfx_upload_texture_data(
		handle_id texture, const u8* data, u32 region_count, const gfx_texture_region* regions) noexcept;

	// ---------- descriptor sets (resource sets) ----------
	struct gfx_resource_set_desc
	{
		const gfx_resource_binding* bindings{nullptr};
		u32 binding_count{0};
	};

	/* Description:
	 *   Create an instance of a resource set (descriptor set) that holds actual resource handles bound to the layout.
	 * Implementation notes:
	 *   - Vulkan: allocate VkDescriptorSet from pool and write descriptors.
	 *   - GL: create CPU-side container describing binding->resource mapping; during bind, GL calls will bind buffers/textures to expected slots.
	 *   - D3D12/Metal: implement descriptor heap/argument buffer allocation as needed.
	 */
	handle_id gfx_create_resource_set(const gfx_resource_set_desc& desc) noexcept;

	/* Description:
	 *   Update bindings within an existing resource set.
	 * Implementation notes:
	 *   - Vulkan: vkUpdateDescriptorSets.
	 *   - GL: update CPU-side mapping; no immediate GPU-side object; binding is performed when set is bound.
	 *   - D3D12/Metal: update descriptor heap entries / argument buffer contents.
	 */
	void gfx_update_resource_set(
		handle_id set_handle, u32 binding_count, const gfx_resource_binding* bindings) noexcept;

	/* Description:
	 *   Destroy a resource set and free any allocated descriptor objects.
	 * Implementation notes:
	 *   - Vulkan: free descriptor set to pool if applicable.
	 *   - GL: free CPU-side container.
	 *   - D3D12/Metal: free-associated descriptor heap slots or argument buffers.
	 */
	void gfx_destroy_resource_set(handle_id set_handle) noexcept;

	// ---------- framebuffers / render passes ----------
	// Abstract simple render pass with attachments
	enum struct gfx_load_op : u8 { LOAD = 0, CLEAR = 1, DONT_CARE = 2 };

	enum struct gfx_store_op : u8 { STORE = 0, DONT_CARE = 1 };

	struct gfx_attachment_desc
	{
		gfx_texture_format format{};
		gfx_load_op load{gfx_load_op::CLEAR};
		gfx_store_op store{gfx_store_op::STORE};
	};

	struct gfx_render_pass_desc
	{
		const gfx_attachment_desc* color_attachments{nullptr};
		u32 color_attachment_count{0};
		bool has_depth{false};
	};

	/* Description:
	 *   Create an abstract render-pass description describing attachments and load/store ops.
	 * Implementation notes:
	 *   - Vulkan: create VkRenderPass with attachment descriptions and subpass.
	 *   - GL: store as metadata; when beginning a GL render-pass emulate load/clear/store behavior by binding FBOs and calling glClear as needed.
	 *   - D3D12/Metal: translate to appropriate render pass/encoders or emulation.
	 */
	handle_id gfx_create_render_pass(const gfx_render_pass_desc& desc) noexcept;

	/* Description:
	 *   Destroy the render-pass metadata or native render-pass object.
	 * Implementation notes:
	 *   - Vulkan: vkDestroyRenderPass.
	 *   - GL: free CPU-side structure.
	 *   - D3D12/Metal: release native objects if any.
	 */
	void gfx_destroy_render_pass(handle_id rp) noexcept;

	struct gfx_framebuffer_desc
	{
		handle_id* color_textures{nullptr};
		u32 color_count{0};
		handle_id depth_texture{};
		u32 width{0};
		u32 height{0};
		handle_id render_pass{};
	};

	/* Description:
	 *   Create or allocate a framebuffer bound to a render pass (group of attachments).
	 * Implementation notes:
	 *   - GL: create FBO and attach GL textures; validate completeness.
	 *   - Vulkan: use VkFramebuffer tied to VkRenderPass.
	 *   - D3D12/Metal: create equivalent render target views / descriptor sets.
	 */
	handle_id gfx_create_framebuffer(const gfx_framebuffer_desc& desc) noexcept;

	/* Description:
	 *   Destroy a framebuffer and free native attachments if owned.
	 * Implementation notes:
	 *   - GL: glDeleteFramebuffers and detach resources.
	 *   - Vulkan/D3D12/Metal: destroy framebuffer object and cleanup.
	 */
	void gfx_destroy_framebuffer(handle_id fb) noexcept;

	// Begin / end render pass (on current command buffer or immediate context)
	struct gfx_clear_value
	{
		float r{0}, g{0}, b{0}, a{0};
		float depth{0};
		u32 stencil{0};
	};

	/* Description:
	 *   Begin a render pass targeting a framebuffer and apply clear values where requested.
	 * Implementation notes:
	 *   - Vulkan: cmdBeginRenderPass with clear values.
	 *   - GL: bind the FBO, set draw buffers, call glClear for attachments with load==CLEAR.
	 *   - D3D12/Metal: set render targets and clear as appropriate.
	 */
	void gfx_cmd_begin_render_pass(
		handle_id framebuffer, const gfx_clear_value* clear_values, u32 clear_count) noexcept;

	/* Description:
	 *   End the current render pass / finish render-target work on the command buffer or immediate context.
	 * Implementation notes:
	 *   - Vulkan: cmdEndRenderPass.
	 *   - GL: unbind FBO or flush if necessary; emulate store operations if post-processing required.
	 *   - D3D12/Metal: end encoder.
	 */
	void gfx_cmd_end_render_pass() noexcept;

	// ---------- command buffer & drawing ----------
	// We provide both an immediate path and a record/submit path.
	// Command buffers are optional for backends that don't need them.
	/* Description:
	 *   Allocate or create a command buffer object for recording GPU commands.
	 * Implementation notes:
	 *   - Vulkan/D3D12/Metal: allocate native command buffer.
	 *   - GL: return a dummy/opaque handle representing immediate context or a recorded list that will replay on submit (optional).
	 */
	handle_id gfx_create_command_buffer() noexcept;

	/* Description:
	 *   Destroy/free a command buffer.
	 * Implementation notes:
	 *   - Vulkan: free the command buffer to pool.
	 *   - GL: free any recorded command-list or no-op for immediate-mode.
	 */
	void gfx_destroy_command_buffer(handle_id cb) noexcept;

	/* Description:
	 *   Begin recording commands into the provided command buffer.
	 * Implementation notes:
	 *   - Vulkan/D3D12: vkBeginCommandBuffer / ID3D12GraphicsCommandList->Reset.
	 *   - GL: start recording to an in-memory command list if you support deferred execution; otherwise prepare immediate context.
	 */
	void gfx_cmd_begin(handle_id cb) noexcept;

	/* Description:
	 *   End recording of commands.
	 * Implementation notes:
	 *   - Vulkan/D3D12: vkEndCommandBuffer / Close command list.
	 *   - GL: finalize recorded list or no-op.
	 */
	void gfx_cmd_end(handle_id cb) noexcept;

	/* Description:
	 *   Bind a pipeline to the command buffer (or immediate context).
	 * Implementation notes:
	 *   - Vulkan/D3D12/Metal: cmdBindPipeline / PSO set.
	 *   - GL: glUseProgram(program) and bind the internally-created VAO stored in the pipeline object.
	 */
	void gfx_cmd_bind_pipeline(handle_id cb, handle_id pipeline) noexcept;

	/* Description:
	 *   Bind one or more vertex buffers to the command buffer for use in draw calls.
	 *   The first_binding parameter specifies the first vertex input binding index.
	 *   Each bound buffer corresponds to a vertex binding described in the pipeline’s vertex input layout.
	 * Implementation notes:
	 *   - Vulkan: vkCmdBindVertexBuffers(cmd, firstBinding, count, buffers, offsets)
	 *   - D3D12: IASetVertexBuffers(firstBinding, count, views)
	 *   - Metal: for each binding, setVertexBuffer:offset:atIndex:
	 *   - GL: if using VAOs, glBindVertexBuffer(binding, buffer, offset, stride)
	 */
	void gfx_cmd_bind_vertex_buffers(
		handle_id cmd, u32 first_binding, u32 binding_count, const handle_id* vertex_buffers,
		const u64* offsets) noexcept;

	/* Description:
	 *   Bind an index buffer for indexed draws.
	 * Implementation notes:
	 *   - GL: bind GL_ELEMENT_ARRAY_BUFFER to VAO or context.
	 *   - Vulkan/D3D12: bind index buffer on command buffer with offset and format.
	 */
	void gfx_cmd_bind_index_buffer(handle_id cb, handle_id index_buffer, u32 index_type) noexcept;

	/* Description:
	 *   Bind a resource set (descriptor set) to the pipeline at a given set index.
	 * Implementation notes:
	 *   - Vulkan: cmdBindDescriptorSets.
	 *   - GL: perform glBindBufferRange/glBindSampler/glActiveTexture + glBindTexture for each binding based on stored mapping.
	 *   - D3D12/Metal: set descriptor heaps or argument buffers accordingly.
	 */
	void gfx_cmd_bind_resource_set(handle_id cb, handle_id pipeline, handle_id set, u32 set_index) noexcept;

	/* Description:
	 *   Issue an (instanced) non-indexed draw call.
	 * Implementation notes:
	 *   - Vulkan: cmdDraw.
	 *   - GL: glDrawArraysInstanced using currently bound VAO and program.
	 *   - D3D12/Metal: DrawInstanced call on the command list/encoder.
	 */
	void gfx_cmd_draw(
		handle_id cb, u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) noexcept;

	/* Description:
	 *   Issue an (instanced) indexed draw call.
	 * Implementation notes:
	 *   - Vulkan: cmdDrawIndexed.
	 *   - GL: glDrawElementsInstanced with bound index buffer and VAO.
	 *   - D3D12/Metal: DrawIndexedInstanced equivalent.
	 */
	void gfx_cmd_draw_indexed(
		handle_id cb, u32 index_count, u32 instance_count = 1, u32 first_index = 0, i32 vertex_offset = 0) noexcept;

	/* Description:
	 *   Copy a region of one buffer to another (GPU-side copy).
	 * Implementation notes:
	 *   - Vulkan: cmdCopyBuffer.
	 *   - GL: glCopyBufferSubData.
	 *   - D3D12/Metal: CopyBufferRegion/Blit equivalents.
	 */
	void gfx_cmd_copy_buffer(
		handle_id cb, handle_id src, handle_id dst, u64 src_offset, u64 dst_offset, u64 size) noexcept;

	/* Description:
	 */
	void gfx_cmd_dispatch(handle_id cb, u32 x, u32 y, u32 z) noexcept;

	/* Description:
	 *   Submit the command buffer for execution (or execute immediate commands).
	 * Implementation notes:
	 *   - Vulkan/D3D12/Metal: submit to a queue with signaling sync primitives.
	 *   - GL: if recorded, replay; otherwise ensure commands have been flushed.
	 */
	void gfx_submit(handle_id cb) noexcept; // submit and execute or immediate execute

	/* Description:
	 *   Block until GPU is idle (synchronization utility).
	 * Implementation notes:
	 *   - Vulkan: vkDeviceWaitIdle.
	 *   - GL: glFinish (note: glFinish waits for GL command completion).
	 *   - D3D12/Metal: wait on fence/command queue.
	 */
	void gfx_wait_idle() noexcept;

	// ---------- synchronization / barriers ----------

	enum struct gfx_shader_access : u32
	{
		NONE     = 0u,
		READ     = bx_bit(1),
		WRITE    = bx_bit(2),
		TRANSFER = bx_bit(3)
	};

	struct gfx_memory_barrier
	{
		gfx_shader_access src_access{};
		gfx_shader_access dst_access{};
		gfx_shader_stage src_stage{};
		gfx_shader_stage dst_stage{};
	};

	/* Description:
	 *   Insert a memory or synchronization barrier to ensure visibility of memory operations between GPU stages.
	 *   This ensures that writes performed by shaders or transfer operations are visible to subsequent reads/writes.
	 *
	 * Implementation notes:
	 *   - Vulkan: issue vkCmdPipelineBarrier with appropriate stage and access masks.
	 *   - D3D12/Metal: issue resource barriers or encoder synchronization points.
	 *   - GL: call glMemoryBarrier() with appropriate flags derived from src/dst access and shader stages.
	 *
	 * OpenGL implementation:
	 *   - Map gfx_shader_access to GL barrier bits:
	 *       READ/WRITE -> GL_SHADER_IMAGE_ACCESS_BARRIER_BIT or GL_TEXTURE_FETCH_BARRIER_BIT
	 *       TRANSFER   -> GL_BUFFER_UPDATE_BARRIER_BIT
	 *   - Example:
	 *       GLbitfield bits = 0;
	 *       if (dst_access & gfx_shader_access::WRITE) bits |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
	 *       if (dst_access & gfx_shader_access::READ)  bits |= GL_TEXTURE_FETCH_BARRIER_BIT;
	 *       glMemoryBarrier(bits);
	 */
	void gfx_pipeline_barrier(handle_id cb, const gfx_memory_barrier& barrier) noexcept;


	// ---------- convenience drawing path (immediate) ----------

	/* Description:
	 *   Return a handle representing the current "immediate" command buffer.
	 *   This allows issuing commands without explicitly creating or managing a command buffer object.
	 *
	 * Implementation notes:
	 *   - Vulkan/D3D12/Metal: return a handle to a transient or default command buffer that records immediately.
	 *   - GL: return a dummy handle (e.g. 0) since GL operates in immediate mode.
	 *
	 * OpenGL implementation:
	 *   - Simply return a static dummy handle (e.g. {0}) since no explicit command buffer exists.
	 */
	handle_id gfx_immediate_command_buffer() noexcept;

	/* Description:
	 *   Return a handle representing the default framebuffer or swap-chain backbuffer.
	 *   This is typically used when beginning a render pass targeting the main display.
	 *
	 * Implementation notes:
	 *   - Vulkan/D3D12/Metal: return a handle to the current swap-chain image/framebuffer.
	 *   - GL: return a dummy handle (e.g. 0) representing the default framebuffer.
	 *
	 * OpenGL implementation:
	 *   - Return {0}, since the default framebuffer is bound by default.
	 */
	handle_id gfx_default_framebuffer() noexcept;
}

#endif //BX_HPP
