#include <bxl_internal.hpp>

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <atomic>

struct gl_pipeline_layout_t
{
	std::vector<bx::handle_id> set_layouts;
};

struct gl_pipeline_t
{
	GLuint program{ 0 };
	GLuint vao{ 0 };
	bx::gfx_topology_t topology{};
	bx::gfx_raster_state_t raster{};
	std::vector<bx::gfx_color_blend_attachment_t> blend_attachments{};
	bx::handle_id layout{};
};

struct gl_buffer_t
{
	GLuint id{ 0 };
	GLenum target{ GL_ARRAY_BUFFER };
	bx::gfx_buffer_usage_t usage{};
	bx::gfx_memory_usage_t mem_usage{};
	u64 size{ 0 };
};

struct gl_texture_t
{
	GLuint id{ 0 };
	bx::gfx_texture_type_t type{};
	bx::gfx_texture_format_t format{};
	u32 width{ 0 };
	u32 height{ 0 };
	u32 depth{ 1 };
	u8 mip_levels{ 1 };
};

struct gl_renderpass_t
{
	std::vector<bx::gfx_attachment_desc_t> color_attachments;
	bool has_depth{ false };
};

struct gl_framebuffer_t
{
	GLuint id{ 0 };
	std::vector<bx::handle_id> color_textures;
	bx::handle_id depth_texture{ 0 };
	u32 width{ 0 };
	u32 height{ 0 };
	bx::handle_id render_pass{ 0 };
};

static std::unordered_map<bx::handle_id, gl_renderpass_t> g_render_passes;
static std::unordered_map<bx::handle_id, gl_framebuffer_t> g_framebuffers;
static std::unordered_map<bx::handle_id, gl_buffer_t> g_buffers;
static std::unordered_map<bx::handle_id, gl_texture_t> g_textures;
static std::unordered_map<bx::handle_id, gl_pipeline_layout_t> g_pipeline_layouts;
static std::unordered_map<bx::handle_id, gl_pipeline_t> g_pipelines;

static std::atomic<u32> g_next_render_pass_id{ 1 };
static std::atomic<u32> g_next_framebuffer_id{ 1 };
static std::atomic<u32> g_next_buffer_id{ 1 };
static std::atomic<u32> g_next_texture_id{ 1 };
static std::atomic<u32> g_next_pipeline_layout_id{ 1 };
static std::atomic<u32> g_next_pipeline_id{ 1 };

// Current bound immediate state
static bx::handle_id g_current_framebuffer = 0;
static bx::handle_id g_current_pipeline = 0;
static bx::handle_id g_current_index_buffer = 0;

// convenience: index buffer index size in bytes (for immediate-binding path)
static GLenum g_current_index_type = GL_UNSIGNED_INT;

cstring bx::gfx_backend_name() noexcept
{
#ifdef BXL_GFX_OPENGLES
	return "opengles";
#else
	return "opengl";
#endif
}

const bx::gfx_features_t& bx::gfx_get_features() noexcept
{
	static bx::gfx_features_t features{};
	return features;
}

static void gl_set_debug_name(GLenum identifier, GLuint name, GLsizei length, cstring label) noexcept
{
	if (glObjectLabel)
		glObjectLabel(GL_TEXTURE, name, length, label);
	else
		bx_logd("Context does not support glObjectLabel");
}

void bx::gfx_push_debug_group(cstring name) noexcept
{
	if (glPushDebugGroup)
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
}

void bx::gfx_pop_debug_group() noexcept
{
	if (glPopDebugGroup)
		glPopDebugGroup();
}

void bx::gfx_insert_debug_marker(cstring name) noexcept
{
	if (glDebugMessageInsert)
		glDebugMessageInsert(
			GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
			GL_DEBUG_SEVERITY_NOTIFICATION, -1, name);
}

bx::handle_id bx::gfx_create_shader(const gfx_shader_desc_t& desc) noexcept
{
	GLenum stage = 0;
	switch (desc.stage)
	{
	case gfx_shader_stage_t::VERTEX:
		stage = GL_VERTEX_SHADER;
		break;
	case gfx_shader_stage_t::FRAGMENT:
		stage = GL_FRAGMENT_SHADER;
		break;
	case gfx_shader_stage_t::GEOMETRY:
		stage = GL_GEOMETRY_SHADER;
		break;
	case gfx_shader_stage_t::COMPUTE:
		stage = GL_COMPUTE_SHADER;
		break;
	default:
		bx_logd("gfx_create_shader: Unknown shader stage.");
		return invalid_handle;
	}
	const GLuint shader = glCreateShader(stage);
	if (!shader)
	{
		bx_logd("gfx_create_shader: Failed to create GL shader object.");
		return invalid_handle;
	}

	// --- load source ---
	std::string source_code;
#ifdef BXL_GFX_OPENGLES
	source_code = "#version 310 es\n";
#else
	source_code = "#version 460 core\n";
#endif

	if (!desc.source && desc.filepath)
	{
		// load from file path
		std::ifstream file(desc.filepath);
		if (!file.is_open())
		{
			std::string msg = "gfx_create_shader: Failed to open file: ";
			msg += desc.filepath;
			bx_loge(msg.c_str());
			glDeleteShader(shader);
			return 0; // invalid handle
		}
		std::string file_src{};
		file_src.assign(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		source_code += file_src;
	}
	else
	{
		source_code += desc.source;
	}

	cstring src_ptr = source_code.c_str();

	// --- handle binary shader case ---
	if (desc.src_bin.data && desc.src_bin.size > 0)
	{
#ifdef GL_ARB_gl_spirv
		if (desc.lang == gfx_shader_lang_t::SPIR_V)
		{
			glShaderBinary(
				1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
				desc.src_bin.data, static_cast<GLsizei>(desc.src_bin.size));
			glSpecializeShaderARB(shader, "main", 0, nullptr, nullptr);
		}
		else
#endif
		{
			bx_loge("gfx_create_shader: Binary shaders require SPIR-V and GL_ARB_gl_spirv.");
			glDeleteShader(shader);
			return invalid_handle;
		}
	}
	else
	{
		if (!src_ptr)
		{
			bx_loge("gfx_create_shader: No source or binary provided.");
			glDeleteShader(shader);
			return invalid_handle;
		}

		glShaderSource(shader, 1, &src_ptr, nullptr);
		glCompileShader(shader);
	}

	// --- check compile status ---
	GLint ok = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		GLint len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetShaderInfoLog(shader, len, &len, &log[0]);

		std::string msg = "Shader compile error: ";
		if (desc.filepath)
		{
			msg += desc.filepath;
			msg += "\n";
		}
		msg += log;
		bx_loge(msg.c_str());

		glDeleteShader(shader);
		return invalid_handle;
	}

	// --- attach debug name (if available) ---
	if (desc.filepath)
		gl_set_debug_name(stage, shader, -1, desc.filepath);
	else if (desc.source)
		gl_set_debug_name(stage, shader, -1, "<inline>");

	// --- pack handle ---
	return handle_t{ shader, stage }.id;
}

void bx::gfx_destroy_shader(const handle_id handle) noexcept
{
	if (handle == 0)
		return;
	glDeleteShader(handle_t{ handle }.data);
}

bx::handle_id bx::gfx_create_resource_set_layout(const gfx_resource_set_layout_desc_t& desc) noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_resource_set_layout(const handle_id handle) noexcept {}

bx::handle_id bx::gfx_create_pipeline_layout(const gfx_pipeline_layout_desc_t& desc) noexcept
{
	gl_pipeline_layout_t layout;

	if (desc.set_layouts)
		layout.set_layouts.assign(desc.set_layouts.data, desc.set_layouts.data + desc.set_layouts.size);

	const u32 id = g_next_pipeline_layout_id.fetch_add(1);
	const handle_id handle = id;

	g_pipeline_layouts[handle] = std::move(layout);
	return handle;
}

void bx::gfx_destroy_pipeline_layout(const handle_id handle) noexcept
{
	g_pipeline_layouts.erase(handle);
}

bx::handle_id bx::gfx_create_pipeline(const gfx_pipeline_desc_t& desc) noexcept
{
	if (!desc.shaders)
	{
		bx_loge("gfx_create_pipeline: no shaders provided");
		return invalid_handle;
	}

	const GLuint program = glCreateProgram();

	// Attach each shader
	for (u8 i = 0; i < desc.shaders.size; ++i)
	{
		const auto& sh = desc.shaders[i];
		const auto shader = handle_t{ sh.handle }.get_data<GLuint>();
		glAttachShader(program, shader);
	}

	// Vertex input configuration is ignored here, because
	// OpenGL handles that via VAOs rather than pipeline creation.
	glLinkProgram(program);

	GLint linked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLint logLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
		std::string log(logLen, '\0');
		glGetProgramInfoLog(program, logLen, &logLen, &log[0]);
		log = "Pipeline link error: " + log;
		bx_loge(log.c_str());
		glDeleteProgram(program);
		return invalid_handle;
	}

	// Detach shaders after linking (keep them alive in case they are reused)
	for (u8 i = 0; i < desc.shaders.size; ++i)
	{
		const auto& sh = desc.shaders[i];
		const auto shader = handle_t{ sh.handle }.get_data<GLuint>();
		glDetachShader(program, shader);
	}

	// Create VAO and configure vertex input
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	for (u32 i = 0; i < desc.vertex_input.attributes.size; ++i)
	{
		const auto& attr = desc.vertex_input.attributes[i];

		glEnableVertexAttribArray(attr.location);

		GLenum type = GL_FLOAT;
		GLboolean normalized = GL_FALSE;
		GLint size = 1;

		switch (attr.format)
		{
		case gfx_attribute_format_t::FLOAT:
			size = 1;
			type = GL_FLOAT;
			break;
		case gfx_attribute_format_t::FLOAT2:
			size = 2;
			type = GL_FLOAT;
			break;
		case gfx_attribute_format_t::FLOAT3:
			size = 3;
			type = GL_FLOAT;
			break;
		case gfx_attribute_format_t::FLOAT4:
			size = 4;
			type = GL_FLOAT;
			break;
		case gfx_attribute_format_t::UINT8_4_NORM:
			size = 4;
			type = GL_UNSIGNED_BYTE;
			normalized = GL_TRUE;
			break;
		default:
			break;
		}

		// We can't bind buffers yet — use binding slot (GL 4.3+) if available
		// or assume binding 0 for simple GL 3.x path:
		const auto binding = attr.binding;

		const auto stride = (binding < desc.vertex_input.bindings.size)
			? desc.vertex_input.bindings[binding].stride
			: 0;

		glVertexAttribPointer(
			attr.location, size, type, normalized, static_cast<GLsizei>(stride),
			reinterpret_cast<vptr>(static_cast<uintptr_t>(attr.offset)));
		glVertexAttribDivisor(
			attr.location,
			(binding < desc.vertex_input.bindings.size)
			? desc.vertex_input.bindings[binding].input_rate_per_vertex_or_instance
			: 0);
	}

	glBindVertexArray(0);

	gl_set_debug_name(GL_PROGRAM, program, -1, desc.name);
	gl_set_debug_name(GL_VERTEX_ARRAY, vao, -1, desc.name);

	// Create pipeline record
	gl_pipeline_t pipeline{};
	pipeline.program = program;
	pipeline.vao = vao;
	pipeline.topology = desc.topology;
	pipeline.raster = desc.raster;
	pipeline.layout = desc.layout;
	//pipeline.vertex_input = desc.vertex_input;

	if (desc.color_attachments)
	{
		pipeline.blend_attachments.assign(
			desc.color_attachments.data,
			desc.color_attachments.data + desc.color_attachments.size);
	}

	const u32 id = g_next_pipeline_id.fetch_add(1);
	const handle_id handle = id;
	g_pipelines[handle] = std::move(pipeline);

	return handle;
}

void bx::gfx_destroy_pipeline(const handle_id handle) noexcept
{
	const auto it = g_pipelines.find(handle);
	if (it != g_pipelines.end())
	{
		glDeleteProgram(it->second.program);
		if (it->second.vao)
			glDeleteVertexArrays(1, &it->second.vao);
		g_pipelines.erase(it);
	}
}

static GLenum gl_buffer_target_from_usage(const bx::gfx_buffer_usage_t usage)
{
	if (bx_enum_any(usage & bx::gfx_buffer_usage_t::VERTEX))
		return GL_ARRAY_BUFFER;
	if (bx_enum_any(usage & bx::gfx_buffer_usage_t::INDEX))
		return GL_ELEMENT_ARRAY_BUFFER;
	if (bx_enum_any(usage & bx::gfx_buffer_usage_t::UNIFORM))
		return GL_UNIFORM_BUFFER;
	if (bx_enum_any(usage & bx::gfx_buffer_usage_t::STORAGE))
		return GL_SHADER_STORAGE_BUFFER;
	return GL_ARRAY_BUFFER;
}

static GLenum gl_usage_hint_from_memory(const bx::gfx_memory_usage_t mem)
{
	switch (mem)
	{
	case bx::gfx_memory_usage_t::GPU_ONLY:
		return GL_STATIC_DRAW;
	case bx::gfx_memory_usage_t::CPU_TO_GPU:
		return GL_DYNAMIC_DRAW;
	case bx::gfx_memory_usage_t::GPU_TO_CPU:
		return GL_STREAM_READ;
	default:
		return GL_STATIC_DRAW;
	}
}

bx::handle_id bx::gfx_create_buffer(const gfx_buffer_desc_t& desc) noexcept
{
	GLuint id = 0;
	glGenBuffers(1, &id);

	const GLenum target = gl_buffer_target_from_usage(desc.usage);
	const GLenum usage = gl_usage_hint_from_memory(desc.memory_usage);

	glBindBuffer(target, id);
	glBufferData(target, static_cast<GLsizeiptr>(desc.size), desc.data, usage);
	glBindBuffer(target, 0);

	gl_buffer_t buffer{};
	buffer.id = id;
	buffer.target = target;
	buffer.usage = desc.usage;
	buffer.mem_usage = desc.memory_usage;
	buffer.size = desc.size;

	gl_set_debug_name(target, id, -1, desc.name);

	const u32 new_id = g_next_buffer_id.fetch_add(1);
	const handle_id handle = new_id;
	g_buffers[handle] = buffer;
	return handle;
}

void bx::gfx_destroy_buffer(const handle_id handle) noexcept
{
	const auto it = g_buffers.find(handle);
	if (it != g_buffers.end())
	{
		glDeleteBuffers(1, &it->second.id);
		g_buffers.erase(it);
	}
}

u8* bx::gfx_map_buffer(const handle_id handle, const u64 offset, const u64 size) noexcept
{
	const auto it = g_buffers.find(handle);
	if (it == g_buffers.end())
		return nullptr;

	const gl_buffer_t& buf = it->second;
	glBindBuffer(buf.target, buf.id);

	GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;
	if (buf.mem_usage == gfx_memory_usage_t::GPU_TO_CPU)
		access = GL_MAP_READ_BIT;

	vptr ptr = glMapBufferRange(buf.target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), access);
	glBindBuffer(buf.target, 0);
	return static_cast<u8*>(ptr);
}

void bx::gfx_unmap_buffer(const handle_id handle) noexcept
{
	const auto it = g_buffers.find(handle);
	if (it == g_buffers.end())
		return;

	const gl_buffer_t& buf = it->second;
	glBindBuffer(buf.target, buf.id);
	glUnmapBuffer(buf.target);
	glBindBuffer(buf.target, 0);
}

void bx::gfx_update_buffer(const handle_id handle, const u64 dst_offset, cvptr src, const u64 size) noexcept
{
	const auto it = g_buffers.find(handle);
	if (it == g_buffers.end())
		return;

	const gl_buffer_t& buf = it->second;
	glBindBuffer(buf.target, buf.id);
	glBufferSubData(buf.target, static_cast<GLintptr>(dst_offset), static_cast<GLsizeiptr>(size), src);
	glBindBuffer(buf.target, 0);
}

static GLenum gl_target_from_type(bx::gfx_texture_type_t type)
{
	using namespace bx;
	switch (type)
	{
	case gfx_texture_type_t::TEX2D:
		return GL_TEXTURE_2D;
	case gfx_texture_type_t::TEX3D:
		return GL_TEXTURE_3D;
	case gfx_texture_type_t::TEX_CUBE:
		return GL_TEXTURE_CUBE_MAP;
	default:
		return GL_TEXTURE_2D;
	}
}

static GLenum gl_format_from_texture_format(bx::gfx_texture_format_t fmt, GLenum& internal, GLenum& type)
{
	using namespace bx;
	switch (fmt)
	{
	case gfx_texture_format_t::R8U_NORM:
		internal = GL_R8;
		type = GL_UNSIGNED_BYTE;
		return GL_RED;
	case gfx_texture_format_t::RG8U_NORM:
		internal = GL_RG8;
		type = GL_UNSIGNED_BYTE;
		return GL_RG;
	case gfx_texture_format_t::RGBA8U_NORM:
		internal = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		return GL_RGBA;
	case gfx_texture_format_t::RGBA16F:
		internal = GL_RGBA16F;
		type = GL_HALF_FLOAT;
		return GL_RGBA;
	case gfx_texture_format_t::DEPTH24_STENCIL8:
		internal = GL_DEPTH24_STENCIL8;
		type = GL_UNSIGNED_INT_24_8;
		return GL_DEPTH_STENCIL;
	default:
		internal = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		return GL_RGBA;
	}
}

bx::handle_id bx::gfx_create_texture(const gfx_texture_desc_t& desc) noexcept
{
	GLuint tex = 0;
	glGenTextures(1, &tex);

	const GLenum target = gl_target_from_type(desc.type);
	glBindTexture(target, tex);

	// Determine GL formats
	GLenum internal_fmt = 0, pixel_fmt = 0, type = 0;
	pixel_fmt = gl_format_from_texture_format(desc.format, internal_fmt, type);

	// Allocate immutable storage
	switch (target)
	{
	case GL_TEXTURE_2D:
		glTexStorage2D(
			target, desc.mip_levels, internal_fmt, static_cast<GLsizei>(desc.width),
			static_cast<GLsizei>(desc.height));
		break;
	case GL_TEXTURE_3D:
		glTexStorage3D(
			target, desc.mip_levels, internal_fmt, static_cast<GLsizei>(desc.width),
			static_cast<GLsizei>(desc.height), static_cast<GLsizei>(desc.depth));
		break;
	case GL_TEXTURE_CUBE_MAP:
		glTexStorage2D(
			GL_TEXTURE_CUBE_MAP, desc.mip_levels, internal_fmt, static_cast<GLsizei>(desc.width),
			static_cast<GLsizei>(desc.height));
		break;
	default:
		break;
	}

	// Sampler/filter settings
	const GLint gl_min = (desc.min_filter == gfx_texture_filter_t::NEAREST)
		? GL_NEAREST_MIPMAP_NEAREST
		: GL_LINEAR_MIPMAP_LINEAR;
	const GLint gl_mag = (desc.mag_filter == gfx_texture_filter_t::NEAREST)
		? GL_NEAREST
		: GL_LINEAR;

	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, gl_min);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, gl_mag);

	const auto wrap_to_gl = [](const gfx_texture_wrap_t w)
		{
			switch (w)
			{
			case gfx_texture_wrap_t::CLAMP_TO_EDGE:
				return GL_CLAMP_TO_EDGE;
			case gfx_texture_wrap_t::REPEAT:
				return GL_REPEAT;
			default:
				return GL_CLAMP_TO_EDGE;
			}
		};

	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_to_gl(desc.wrap_u));
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_to_gl(desc.wrap_v));
	glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap_to_gl(desc.wrap_w));

	glBindTexture(target, 0);

	gl_texture_t texture{};
	texture.id = tex;
	texture.type = desc.type;
	texture.format = desc.format;
	texture.width = desc.width;
	texture.height = desc.height;
	texture.depth = desc.depth;
	texture.mip_levels = desc.mip_levels;

	const u32 new_id = g_next_texture_id.fetch_add(1);
	const handle_id handle = new_id;
	g_textures[handle] = texture;
	return handle;
}

void bx::gfx_destroy_texture(const handle_id handle) noexcept
{
	auto it = g_textures.find(handle);
	if (it != g_textures.end())
	{
		glDeleteTextures(1, &it->second.id);
		g_textures.erase(it);
	}
}

void bx::gfx_upload_texture_data(const handle_id texture, const u8* data, const u32 region_count, const gfx_texture_region_t* regions) noexcept
{
	const auto it = g_textures.find(texture);
	if (it == g_textures.end() || !data || !regions)
		return;

	const gl_texture_t& tex = it->second;
	const GLenum target = gl_target_from_type(tex.type);

	GLenum internal = 0, pixel_fmt = 0, type = 0;
	pixel_fmt = gl_format_from_texture_format(tex.format, internal, type);

	glBindTexture(target, tex.id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (u32 i = 0; i < region_count; ++i)
	{
		const auto& r = regions[i];
		const u8* src = data;

		if (r.row_stride != 0)
			glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(r.row_stride / 4));
		else
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		switch (target)
		{
		case GL_TEXTURE_2D:
			glTexSubImage2D(
				target, r.mip_level,
				r.x, r.y,
				static_cast<GLsizei>(r.width), static_cast<GLsizei>(r.height),
				pixel_fmt, type, src
			);
			break;
		case GL_TEXTURE_CUBE_MAP:
			// Cube faces would need to be specified as regions (you can expand later)
			glTexSubImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X, r.mip_level,
				r.x, r.y,
				static_cast<GLsizei>(r.width), static_cast<GLsizei>(r.height),
				pixel_fmt, type, src
			);
			break;
		case GL_TEXTURE_3D:
			glTexSubImage3D(
				target, r.mip_level,
				r.x, r.y, 0,
				static_cast<GLsizei>(r.width), static_cast<GLsizei>(r.height), static_cast<GLsizei>(tex.depth),
				pixel_fmt, type, src
			);
			break;
		default:
			break;
		}
	}

	glBindTexture(target, 0);
}

bx::handle_id bx::gfx_create_resource_set(const gfx_resource_set_desc_t& desc) noexcept
{
	return handle_id{0};
}

void bx::gfx_update_resource_set(handle_id set_handle, u32 binding_count, const gfx_resource_binding_t* bindings) noexcept {}

void bx::gfx_destroy_resource_set(handle_id set_handle) noexcept {}

bx::handle_id bx::gfx_create_renderpass(const gfx_renderpass_desc_t& desc) noexcept
{
	gl_renderpass_t rp;
	if (desc.color_attachments)
	{
		rp.color_attachments.assign(
			desc.color_attachments.data,
			desc.color_attachments.data + desc.color_attachments.size);
	}
	rp.has_depth = desc.has_depth;

	const u32 id = g_next_render_pass_id.fetch_add(1);
	const handle_id handle = id;
	g_render_passes[handle] = std::move(rp);
	return handle;
}

void bx::gfx_destroy_renderpass(handle_id rp) noexcept
{
	g_render_passes.erase(rp);
}

bx::handle_id bx::gfx_create_framebuffer(const gfx_framebuffer_desc_t& desc) noexcept
{
	GLuint fbo = 0;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	for (u32 i = 0; i < desc.color_textures.size; ++i)
	{
		auto tex_it = g_textures.find(desc.color_textures[i]);
		if (tex_it == g_textures.end())
			continue;

		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
			GL_TEXTURE_2D, tex_it->second.id, 0);
	}

	if (desc.depth_texture)
	{
		const auto depth_it = g_textures.find(desc.depth_texture);
		if (depth_it != g_textures.end())
		{
			const GLenum attachment = (depth_it->second.format == gfx_texture_format_t::DEPTH24_STENCIL8)
				? GL_DEPTH_STENCIL_ATTACHMENT
				: GL_DEPTH_ATTACHMENT;

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, depth_it->second.id, 0);
		}
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		bx_loge("Framebuffer incomplete");
		glDeleteFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return invalid_handle;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	gl_framebuffer_t fb{};
	fb.id = fbo;
	fb.width = desc.width;
	fb.height = desc.height;
	fb.color_textures.assign(desc.color_textures.data, desc.color_textures.data + desc.color_textures.size);
	fb.depth_texture = desc.depth_texture;
	fb.render_pass = desc.render_pass;

	const u32 id = g_next_framebuffer_id.fetch_add(1);
	const handle_id handle = id;
	g_framebuffers[handle] = std::move(fb);
	return handle;
}

void bx::gfx_destroy_framebuffer(handle_id fb) noexcept
{
	auto it = g_framebuffers.find(fb);
	if (it != g_framebuffers.end())
	{
		glDeleteFramebuffers(1, &it->second.id);
		g_framebuffers.erase(it);
	}
}

bx::handle_id bx::gfx_default_framebuffer() noexcept
{
	return handle_id{ 0 };
}

bx::handle_id bx::gfx_immediate_command_buffer() noexcept
{
	return handle_id{ 0 };
}

void bx::gfx_cmd_begin_renderpass(const handle_id framebuffer, const gfx_clear_value_t* clear_values, u32 clear_count) noexcept
{
	if (framebuffer == gfx_default_framebuffer())
	{
		if (clear_values && clear_count > 0)
		{
			glClearColor(clear_values[0].r, clear_values[0].g, clear_values[0].b, clear_values[0].a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}
	}

	// bind FBO
	auto it_fb = g_framebuffers.find(framebuffer);
	if (it_fb == g_framebuffers.end())
	{
		// bind default framebuffer (0) if invalid
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		g_current_framebuffer = 0;
		return;
	}

	const gl_framebuffer_t& fb = it_fb->second;
	glBindFramebuffer(GL_FRAMEBUFFER, fb.id);
	g_current_framebuffer = framebuffer;

	// Setup draw buffers based on color attachments count
	GLenum buffers[16] = { 0 };
	for (u32 i = 0; i < fb.color_textures.size() && i < 16; ++i)
		buffers[i] = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i);

	if (!fb.color_textures.empty())
		glDrawBuffers(static_cast<GLsizei>(fb.color_textures.size()), buffers);
	else
		glDrawBuffer(GL_NONE);

	// Apply clears according to render pass metadata (if we have a matching render_pass)
	auto rp_it = g_render_passes.find(fb.render_pass);
	if (rp_it != g_render_passes.end())
	{
		const gl_renderpass_t& rp = rp_it->second;
		GLbitfield mask = 0;

		// Clear color attachments that were requested as CLEAR
		for (u32 i = 0; i < rp.color_attachments.size() && i < fb.color_textures.size(); ++i)
		{
			if (rp.color_attachments[i].load == gfx_load_op_t::CLEAR)
			{
				// Use provided clear_values[i] if available, otherwise 0
				const gfx_clear_value_t* cv = (clear_values && i < clear_count) ? &clear_values[i] : nullptr;
				if (cv)
					glClearBufferfv(GL_COLOR, static_cast<int>(i), &cv->r);
				else
				{
					float zeros[4] = { 0, 0, 0, 0 };
					glClearBufferfv(GL_COLOR, static_cast<int>(i), zeros);
				}
			}
		}

		// Depth clear
		if (rp.has_depth)
		{
			// depth clear value at index after color attachments (if provided)
			const u32 depth_index = !rp.color_attachments.empty() ? static_cast<u32>(rp.color_attachments.size()) : 0;
			const gfx_clear_value_t* cv = (clear_values && depth_index < clear_count)
				? &clear_values[depth_index]
				: nullptr;
			if (cv)
			{
				glClearBufferfv(GL_DEPTH, 0, &cv->depth);
				// stencil if requested
				if (cv->stencil != 0)
					glClearBufferiv(GL_STENCIL, 0, reinterpret_cast<const GLint*>(&cv->stencil));
			}
		}
	}
	else
	{
		// No render-pass metadata -> default behavior: if clear_values provided, clear color/depth accordingly
		if (clear_values && clear_count > 0)
		{
			// clear first color buffer
			glClearColor(clear_values[0].r, clear_values[0].g, clear_values[0].b, clear_values[0].a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}
	}
}

void bx::gfx_cmd_end_render_pass() noexcept
{
	// For GL we simply unbind FBO (or bind default). Store previous in current_framebuffer if needed
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	g_current_framebuffer = 0;
}

bx::handle_id bx::gfx_create_command_buffer() noexcept
{
	return handle_id{0};
}

void bx::gfx_destroy_command_buffer(handle_id cb) noexcept {}

void bx::gfx_cmd_begin(handle_id cb) noexcept {}

void bx::gfx_cmd_end(handle_id cb) noexcept {}

void bx::gfx_cmd_bind_pipeline(handle_id cb, handle_id pipeline) noexcept
{
	auto it = g_pipelines.find(pipeline);
	if (it == g_pipelines.end())
		return;

	const gl_pipeline_t& p = it->second;
	glUseProgram(p.program);
	if (p.vao)
		glBindVertexArray(p.vao);
	else
		glBindVertexArray(0);

	g_current_pipeline = pipeline;

	// Apply raster state if desired (culling/depth)
	if (p.raster.cull_enable)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
	if (p.raster.depth_test)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
	glDepthMask(p.raster.depth_write ? GL_TRUE : GL_FALSE);

	// set blend state: simple enable if any attachment has blend_enable
	bool any_blend = false;
	for (auto& a : p.blend_attachments)
		if (a.blend_enable)
		{
			any_blend = true;
			break;
		}
	if (any_blend)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
}

void bx::gfx_cmd_bind_vertex_buffers(handle_id cmd, u32 first_binding, u32 binding_count, const handle_id* vertex_buffers, const u64* offsets) noexcept
{
	// Assume an active VAO was created during pipeline creation.
	// GL 4.3+ supports glBindVertexBuffer directly.
	for (u32 i = 0; i < binding_count; ++i)
	{
		handle_id handle = vertex_buffers[i];
		const auto it = g_buffers.find(handle);
		if (it == g_buffers.end())
			continue;

		GLuint vbo = it->second.id;
		const GLintptr offset = static_cast<GLintptr>(offsets ? offsets[i] : 0);
		// Stride is baked into the vertex input description at pipeline creation time.
		//const GLsizei stride = get_bound_vbo_stride(first_binding + i);
		//glBindVertexBuffer(first_binding + i, vbo, offset, stride);
		glBindVertexBuffer(first_binding + i, vbo, offset, sizeof(float) * 4);
	}
}

void bx::gfx_cmd_bind_index_buffer(handle_id cb, handle_id index_buffer, u32 index_type) noexcept {}

void bx::gfx_cmd_bind_resource_set(handle_id cb, handle_id pipeline_handle, handle_id set_handle, u32 set_index) noexcept {}

static GLenum gl_enum_from_topology(bx::gfx_topology_t t)
{
	switch (t)
	{
	case bx::gfx_topology_t::TRIANGLES:
		return GL_TRIANGLES;
	case bx::gfx_topology_t::TRIANGLE_STRIP:
		return GL_TRIANGLE_STRIP;
	case bx::gfx_topology_t::LINES:
		return GL_LINES;
	case bx::gfx_topology_t::LINE_STRIP:
		return GL_LINE_STRIP;
	case bx::gfx_topology_t::POINTS:
		return GL_POINTS;
	default:
		return GL_TRIANGLES;
	}
}

void bx::gfx_cmd_draw(handle_id cb, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) noexcept
{
	// pick topology from current pipeline
	GLenum mode = GL_TRIANGLES;
	auto p_it = g_pipelines.find(g_current_pipeline);
	if (p_it != g_pipelines.end())
		mode = gl_enum_from_topology(p_it->second.topology);

	if (instance_count <= 1)
		glDrawArrays(mode, static_cast<GLint>(first_vertex), static_cast<GLsizei>(vertex_count));
	else
		glDrawArraysInstanced(
			mode, static_cast<GLint>(first_vertex), static_cast<GLsizei>(vertex_count),
			static_cast<GLsizei>(instance_count));
}

void bx::gfx_cmd_draw_indexed(handle_id cb, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset) noexcept {}

void bx::gfx_cmd_copy_buffer(handle_id cb, handle_id src, handle_id dst, u64 src_offset, u64 dst_offset, u64 size) noexcept {}

void bx::gfx_cmd_dispatch(handle_id cb, u32 x, u32 y, u32 z) noexcept {}

void bx::gfx_submit(handle_id cb) noexcept {}

void bx::gfx_wait_idle() noexcept {}

void bx::gfx_pipeline_barrier(handle_id cb, const gfx_memory_barrier_t& barrier) noexcept {}