#include <bxl_app.hpp>

#include <vulkan/vulkan.h>

// ---------- utilities ----------
cstring bx::gfx_backend_name() bx_noexcept
{
	return "vulkan";
}

// -----------------------------------------------------------------------------
// Debug helpers
// -----------------------------------------------------------------------------
void bx::gfx_set_debug_name(handle_id object, cstring name) bx_noexcept {}

void bx::gfx_push_debug_group(cstring name) bx_noexcept {}

void bx::gfx_pop_debug_group() bx_noexcept {}

void bx::gfx_insert_debug_marker(cstring name) bx_noexcept {}

bx::handle_id bx::gfx_create_shader(const gfx_shader_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_shader(const handle_id handle) bx_noexcept {}

bx::handle_id bx::gfx_create_resource_set_layout(const gfx_resource_set_layout_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_resource_set_layout(const handle_id handle) bx_noexcept {}

bx::handle_id bx::gfx_create_pipeline_layout(const gfx_pipeline_layout_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_pipeline_layout(const handle_id handle) bx_noexcept {}

bx::handle_id bx::gfx_create_pipeline(const gfx_pipeline_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_pipeline(const handle_id handle) bx_noexcept {}

bx::handle_id bx::gfx_create_buffer(const gfx_buffer_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_buffer(const handle_id handle) bx_noexcept {}

u8* bx::gfx_map_buffer(const handle_id handle, const u64 offset, const u64 size) bx_noexcept
{
	return nullptr;
}

void bx::gfx_unmap_buffer(const handle_id handle) bx_noexcept {}

void bx::gfx_update_buffer(const handle_id handle, const u64 dst_offset, const void* src, const u64 size) bx_noexcept {}

bx::handle_id bx::gfx_create_texture(const gfx_texture_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_texture(const handle_id handle) bx_noexcept {}

void bx::gfx_upload_texture_data(
	const handle_id texture, const u8* data, const u32 region_count, const gfx_texture_region* regions) bx_noexcept {}

// Create a new resource set
bx::handle_id bx::gfx_create_resource_set(const gfx_resource_set_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_update_resource_set(
	handle_id set_handle, u32 binding_count, const gfx_resource_binding* bindings) bx_noexcept {}

void bx::gfx_destroy_resource_set(handle_id set_handle) bx_noexcept {}

bx::handle_id bx::gfx_create_render_pass(const gfx_render_pass_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_render_pass(handle_id rp) bx_noexcept {}

bx::handle_id bx::gfx_create_framebuffer(const gfx_framebuffer_desc& desc) bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_framebuffer(handle_id fb) bx_noexcept {}

// ---------- render pass begin/end ----------
void bx::gfx_cmd_begin_render_pass(
	const handle_id framebuffer, const gfx_clear_value* clear_values, u32 clear_count) bx_noexcept {}

void bx::gfx_cmd_end_render_pass() bx_noexcept {}

// ---------- command buffer lifecycle (immediate-mode) ----------
bx::handle_id bx::gfx_create_command_buffer() bx_noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_command_buffer(handle_id cb) bx_noexcept {}

void bx::gfx_cmd_begin(handle_id cb) bx_noexcept
{
	(void)cb; /* no-op for immediate GL */
}

void bx::gfx_cmd_end(handle_id cb) bx_noexcept
{
	(void)cb; /* no-op for immediate GL */
}

// ---------- bind pipeline / index buffer / resource set ----------
void bx::gfx_cmd_bind_pipeline(handle_id /*cb*/, handle_id pipeline) bx_noexcept {}

void bx::gfx_cmd_bind_vertex_buffers(
	handle_id /*cmd*/, u32 first_binding, u32 binding_count, const handle_id* vertex_buffers,
	const u64* offsets) bx_noexcept {}

void bx::gfx_cmd_bind_index_buffer(handle_id /*cb*/, handle_id index_buffer, u32 index_type) bx_noexcept {}

void bx::gfx_cmd_bind_resource_set(
	handle_id /*cb*/, handle_id pipeline_handle, handle_id set_handle, u32 /*set_index*/) bx_noexcept {}

// ---------- draws ----------
void bx::gfx_cmd_draw(
	handle_id /*cb*/, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 /*first_instance*/) bx_noexcept {}

void bx::gfx_cmd_draw_indexed(
	handle_id /*cb*/, u32 index_count, u32 instance_count, u32 first_index, i32 /*vertex_offset*/) bx_noexcept {}

// ---------- buffer copy ----------
void bx::gfx_cmd_copy_buffer(
	handle_id /*cb*/, handle_id src, handle_id dst, u64 src_offset, u64 dst_offset, u64 size) bx_noexcept {}

void bx::gfx_cmd_dispatch(handle_id /*cb*/, u32 x, u32 y, u32 z) bx_noexcept {}

// ---------- submit / wait ----------
void bx::gfx_submit(handle_id /*cb*/) bx_noexcept {}

void bx::gfx_wait_idle() bx_noexcept {}

// -------------------------------------------------------------
// Synchronization / barriers
// -------------------------------------------------------------

void bx::gfx_pipeline_barrier(handle_id /*cb*/, const gfx_memory_barrier& barrier) bx_noexcept {}

// ---------- convenience immediate path (callers) ----------
bx::handle_id bx::gfx_immediate_command_buffer() bx_noexcept
{
	return handle_id{0};
}

bx::handle_id bx::gfx_default_framebuffer() bx_noexcept
{
	return handle_id{0};
}