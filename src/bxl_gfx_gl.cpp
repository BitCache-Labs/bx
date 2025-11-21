#include <bxl_app.hpp>

#include <glad/glad.h>

#include <string>
#include <vector>
#include <fstream>

//#define BXL_GFX_OPENGLES

#define MAX_BOUND_VERTEX_BUFFERS 16

#ifdef BXL_GFX_OPENGLES
static constexpr cstring GLSL_VERSION = "#version 310 es\n";
#else
#ifdef __APPLE__
static constexpr cstring GLSL_VERSION = "#version 410 core\n";
#define BXL_GFX_OPENGLES
#else
static constexpr cstring GLSL_VERSION = "#version 460 core\n";
#endif
#endif

struct gl_shader_t
{
	cstring name{ nullptr };
	GLuint shader{ 0 };
	GLenum stage{ 0 };
};

struct gl_buffer_t
{
	cstring name{ nullptr };
	GLuint id{ 0 };
	GLenum target{ GL_ARRAY_BUFFER };
	bx::gfx_buffer_usage_t usage{};
	bx::gfx_memory_usage_t mem_usage{};
	u64 size{ 0 };
	vptr persistentPtr{ nullptr };
};

struct gl_texture_t
{
	cstring name{ nullptr };
	GLuint id{ 0 };
	bx::gfx_texture_type_t type{};
	bx::gfx_texture_format_t format{};
	u32 width{ 0 };
	u32 height{ 0 };
	u32 depth{ 1 };
	u8 mip_levels{ 1 };
};

struct gl_framebuffer_t
{
	cstring name{ nullptr };
	GLuint id{ 0 };
	std::vector<bx::handle_id> color_textures;
	bx::handle_id depth_texture{ 0 };
	u32 width{ 0 };
	u32 height{ 0 };
	bx::handle_id render_pass{ 0 };
};

struct gl_pipeline_t
{
	cstring name{ nullptr };
	GLuint pipeline{ 0 };
	GLuint vao{ 0 };
	bx::gfx_topology_t topology{};
	bx::gfx_raster_state_t raster{};
	std::vector<bx::gfx_color_blend_attachment_t> blend_attachments{};
};

static bx::handlemap<gl_shader_t> g_shaders{};
static bx::handlemap<gl_buffer_t> g_buffers{};
static bx::handlemap<gl_texture_t> g_textures{};
static bx::handlemap<gl_framebuffer_t> g_framebuffers{};
static bx::handlemap<gl_pipeline_t> g_pipelines{};

// Current bound immediate state
static bx::handle_id g_current_framebuffer = 0;
static bx::handle_id g_current_pipeline = 0;
static bx::handle_id g_current_index_buffer = 0;

// convenience: index buffer index size in bytes (for immediate-binding path)
static GLenum g_current_index_type = GL_UNSIGNED_INT;

static void GLAPIENTRY gl_debug_callback(
	GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const void* userParam)
{
	bx_profile(bx);

	cstring src =
		source == GL_DEBUG_SOURCE_API ? "API" :
		source == GL_DEBUG_SOURCE_WINDOW_SYSTEM ? "WINDOW SYSTEM" :
		source == GL_DEBUG_SOURCE_SHADER_COMPILER ? "SHADER COMPILER" :
		source == GL_DEBUG_SOURCE_THIRD_PARTY ? "THIRD PARTY" :
		source == GL_DEBUG_SOURCE_APPLICATION ? "APPLICATION" :
		source == GL_DEBUG_SOURCE_OTHER ? "OTHER" : "UNKNOWN";

	cstring tp =
		type == GL_DEBUG_TYPE_ERROR ? "ERROR" :
		type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ? "DEPRECATED" :
		type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR ? "UNDEFINED" :
		type == GL_DEBUG_TYPE_PORTABILITY ? "PORTABILITY" :
		type == GL_DEBUG_TYPE_PERFORMANCE ? "PERFORMANCE" :
		type == GL_DEBUG_TYPE_MARKER ? "MARKER" :
		type == GL_DEBUG_TYPE_PUSH_GROUP ? "PUSH_GROUP" :
		type == GL_DEBUG_TYPE_POP_GROUP ? "POP_GROUP" :
		type == GL_DEBUG_TYPE_OTHER ? "OTHER" : "UNKNOWN";

	cstring sev =
		severity == GL_DEBUG_SEVERITY_HIGH ? "HIGH" :
		severity == GL_DEBUG_SEVERITY_MEDIUM ? "MEDIUM" :
		severity == GL_DEBUG_SEVERITY_LOW ? "LOW" :
		severity == GL_DEBUG_SEVERITY_NOTIFICATION ? "NOTIFICATION" :
		"UNKNOWN";

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		bx_logf(bx, "[OpenGL Debug] {} {} ({}) ID={}: {}", src, tp, sev, id, message);
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
	case GL_DEBUG_SEVERITY_LOW:
		bx_logd(bx, "[OpenGL Debug] {} {} ({}) ID={}: {}", src, tp, sev, id, message);
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		//bx_logv(bxl, "[OpenGL Debug] {} {} ({}) ID={}: {}", src, tp, sev, id, message);
		break;
	}
}

bool bx::gfx_init(const bx::app_config_t& config) bx_noexcept
{
	bx_profile(bx);

	//if (glfwExtensionSupported("GL_KHR_debug"))
	if (glDebugMessageCallback)
	{
		// Enable synchronous debug output (makes callbacks happen immediately)
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		// Register the callback
		glDebugMessageCallback(gl_debug_callback, nullptr);

		// Optionally enable all messages
		glDebugMessageControl(
			GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
			0, nullptr, GL_TRUE
		);
	}

	return true;
}

void bx::gfx_shutdown() bx_noexcept
{
	bx_profile(bx);
}

cstring bx::gfx_backend_name() bx_noexcept
{
	bx_profile(bx);

#ifdef BXL_GFX_OPENGLES
	return "opengles";
#else
	return "opengl";
#endif
}

const bx::gfx_features_t& bx::gfx_get_features() bx_noexcept
{
	bx_profile(bx);

	static bx::gfx_features_t features{};
	return features;
}

static void gl_set_debug_name(GLenum identifier, GLuint name, GLsizei length, cstring label) bx_noexcept
{
	bx_profile(bx);

	/*if (glObjectLabel)
		glObjectLabel(identifier, name, length, label);
	else
		bx_logd(bxl, "Context does not support glObjectLabel");*/
}

void bx::gfx_push_debug_group(cstring name) bx_noexcept
{
	bx_profile(bx);

	if (glPushDebugGroup)
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
}

void bx::gfx_pop_debug_group() bx_noexcept
{
	bx_profile(bx);

	if (glPopDebugGroup)
		glPopDebugGroup();
}

void bx::gfx_insert_debug_marker(cstring name) bx_noexcept
{
	bx_profile(bx);

	if (glDebugMessageInsert)
		glDebugMessageInsert(
			GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
			GL_DEBUG_SEVERITY_NOTIFICATION, -1, name);
}

static GLenum gl_get_stage(bx::gfx_shader_stage_t stage)
{
	bx_profile(bx);

	switch (stage)
	{
	case bx::gfx_shader_stage_t::VERTEX:   return GL_VERTEX_SHADER;
	case bx::gfx_shader_stage_t::FRAGMENT: return GL_FRAGMENT_SHADER;
	case bx::gfx_shader_stage_t::GEOMETRY: return GL_GEOMETRY_SHADER;
	case bx::gfx_shader_stage_t::COMPUTE:  return GL_COMPUTE_SHADER;
	default:
		bx_logd(bx, "gfx_create_shader: Unknown shader stage.");
		return 0;
	}
}

static bx::handle_id gl33_create_shader(const bx::gfx_shader_desc_t& desc)
{
	bx_profile(bx);

	const GLenum stage = gl_get_stage(desc.stage);
	if (stage == 0)
		return bx::invalid_handle;

	const GLuint shader = glCreateShader(stage);
	if (!shader)
	{
		bx_logd(bx, "gfx_create_shader: Failed to create GL shader object.");
		return bx::invalid_handle;
	}

	std::string source_code = GLSL_VERSION;
	
	if (!desc.source && desc.filepath)
	{
		std::ifstream file(desc.filepath);
		if (!file.is_open())
		{
			glDeleteShader(shader);
			bx_loge(bx, "gfx_create_shader: Failed to open file {}", desc.filepath);
			return bx::invalid_handle;
		}
		source_code += std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
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
		if (desc.lang == bx::gfx_shader_lang_t::SPIR_V)
		{
			glShaderBinary(
				1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
				desc.src_bin.data, static_cast<GLsizei>(desc.src_bin.size));
			glSpecializeShaderARB(shader, desc.entrypoint, 0, nullptr, nullptr);
		}
		else
#endif
		{
			bx_loge(bx, "gfx_create_shader: Binary shaders require SPIR-V and GL_ARB_gl_spirv.");
			glDeleteShader(shader);
			return bx::invalid_handle;
		}
	}
	else
	{
		if (!src_ptr)
		{
			bx_loge(bx, "gfx_create_shader: No source or binary provided.");
			glDeleteShader(shader);
			return bx::invalid_handle;
		}

		glShaderSource(shader, 1, &src_ptr, nullptr);
		glCompileShader(shader);
	}

	GLint ok = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		GLint len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetShaderInfoLog(shader, len, &len, &log[0]);
		bx_loge(bx, "Shader compile error: {}", log);
		glDeleteShader(shader);
		return bx::invalid_handle;
	}

	gl_set_debug_name(stage, shader, -1, desc.name);

	gl_shader_t glshader{};
	glshader.shader = shader;
	glshader.stage = stage;

	return g_shaders.insert(glshader);
}

static bx::handle_id gl46_create_shader(const bx::gfx_shader_desc_t& desc)
{
	bx_profile(bx);

	const GLenum stage = gl_get_stage(desc.stage);
	if (stage == 0)
		return bx::invalid_handle;

    std::string source_code = "#version 460 core\n";
    if (!desc.source && desc.filepath)
    {
        std::ifstream file(desc.filepath);
        if (!file.is_open())
        {
            bx_loge(bx, "gfx_create_shader: Failed to open file");
            return bx::invalid_handle;
        }
        source_code += std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    else
    {
        source_code += desc.source;
    }

    const char* src_ptr = source_code.c_str();

    GLuint program = glCreateShaderProgramv(stage, 1, &src_ptr);
	//glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
    if (!program)
    {
        bx_loge(bx, "gfx_create_shader: Failed to create shader program");
        return bx::invalid_handle;
    }

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(program, len, &len, &log[0]);
        bx_loge(bx, "Shader program link error: {}", log);
        glDeleteProgram(program);
        return bx::invalid_handle;
    }

    gl_set_debug_name(stage, program, -1, desc.name);

	gl_shader_t glshader{};
	glshader.shader = program;
	glshader.stage = stage;

	return g_shaders.insert(glshader);
}

bx::handle_id bx::gfx_create_shader(const gfx_shader_desc_t& desc) bx_noexcept
{
	bx_profile(bx);

#ifdef BXL_GFX_OPENGLES
	return gl33_create_shader(desc);
#else
	return gl46_create_shader(desc);
#endif
}

void bx::gfx_destroy_shader(const handle_id handle) bx_noexcept
{
	bx_profile(bx);

	auto glsh = g_shaders.get(handle);
	if (!glsh) return;

#ifdef BXL_GFX_OPENGLES
	glDeleteShader(glsh->shader);
#else
	glDeleteProgram(glsh->shader);
#endif

	g_shaders.remove(handle);
}

static GLenum gl_buffer_target_from_usage(const bx::gfx_buffer_usage_t usage)
{
	bx_profile(bx);

	switch (usage)
	{
	case bx::gfx_buffer_usage_t::VERTEX: return GL_ARRAY_BUFFER;
	case bx::gfx_buffer_usage_t::INDEX: return GL_ELEMENT_ARRAY_BUFFER;
	case bx::gfx_buffer_usage_t::UNIFORM: return GL_UNIFORM_BUFFER;
	case bx::gfx_buffer_usage_t::STORAGE: return GL_SHADER_STORAGE_BUFFER;
	default:
		return GL_ARRAY_BUFFER;
	}
}

static GLenum gl_usage_hint_from_memory(const bx::gfx_memory_usage_t mem)
{
	bx_profile(bx);

	switch (mem)
	{
	case bx::gfx_memory_usage_t::GPU_ONLY: return GL_STATIC_DRAW;
	case bx::gfx_memory_usage_t::CPU_TO_GPU: return GL_DYNAMIC_DRAW;
	case bx::gfx_memory_usage_t::GPU_TO_CPU: return GL_STREAM_READ;
	default:
		return GL_STATIC_DRAW;
	}
}

static GLbitfield gl_storage_flags_from_memory(const bx::gfx_memory_usage_t mem)
{
	bx_profile(bx);

	switch (mem)
	{
	case bx::gfx_memory_usage_t::GPU_ONLY:
		return 0; // No CPU access, immutable GPU-only buffer

	case bx::gfx_memory_usage_t::CPU_TO_GPU:
		return GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_MAP_COHERENT_BIT |
			GL_DYNAMIC_STORAGE_BIT;

	case bx::gfx_memory_usage_t::GPU_TO_CPU:
		return GL_MAP_READ_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_DYNAMIC_STORAGE_BIT;

	default:
		return 0;
	}
}

static bx::handle_id gl33_create_buffer(const bx::gfx_buffer_desc_t& desc)
{
	bx_profile(bx);

	const GLenum target = gl_buffer_target_from_usage(desc.usage);

	GLuint id = 0;
	glGenBuffers(1, &id);
	glBindBuffer(target, id);

	vptr persistentPtr = nullptr;

	if (glBufferStorage || glBufferStorageEXT)
	{
		GLbitfield flags = gl_storage_flags_from_memory(desc.memory_usage);
		if (glBufferStorage)
			glBufferStorage(target, (GLsizeiptr)desc.size, desc.data, flags);
		else if (glBufferStorageEXT)
			glBufferStorageEXT(target, (GLsizeiptr)desc.size, desc.data, flags);

		if (flags & (GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT))
		{
			persistentPtr = glMapBufferRange(target, 0, static_cast<GLsizeiptr>(desc.size), flags);
		}
	}
	else
	{
		GLenum hint = gl_usage_hint_from_memory(desc.memory_usage);
		if (desc.memory_usage == bx::gfx_memory_usage_t::CPU_TO_GPU)
		{
			// Streaming optimization
			glBufferData(target, (GLsizeiptr)desc.size, nullptr, hint);

			if (desc.data)
			{
				// Perform unsynchronized map for initial data upload
				persistentPtr = glMapBufferRange(
					target, 0, (GLsizeiptr)desc.size,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

				if (persistentPtr && desc.data)
					memcpy(persistentPtr, desc.data, desc.size);

				glUnmapBuffer(target);
			}
		}
		else
		{
			glBufferData(target, (GLsizeiptr)desc.size, desc.data, hint);
		}
	}

	glBindBuffer(target, 0);

	gl_buffer_t glbuffer{};
	glbuffer.id = id;
	glbuffer.target = target;
	glbuffer.usage = desc.usage;
	glbuffer.mem_usage = desc.memory_usage;
	glbuffer.size = desc.size;
	glbuffer.persistentPtr = persistentPtr;

	gl_set_debug_name(target, id, -1, desc.name);

	return g_buffers.insert(glbuffer);
}

static bx::handle_id gl46_create_buffer(const bx::gfx_buffer_desc_t& desc)
{
	bx_profile(bx);

	GLuint id = 0;
	glCreateBuffers(1, &id);

	const GLsizeiptr size = (GLsizeiptr)desc.size;
	GLbitfield flags = gl_storage_flags_from_memory(desc.memory_usage);

	glNamedBufferStorage(id, size, desc.data, flags);

	void* persistentPtr = nullptr;
	if (flags & (GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT))
	{
		persistentPtr = glMapNamedBufferRange(id, 0, size, flags);
	}

	gl_buffer_t glbuffer{};
	glbuffer.id = id;
	glbuffer.target = gl_buffer_target_from_usage(desc.usage);
	glbuffer.usage = desc.usage;
	glbuffer.mem_usage = desc.memory_usage;
	glbuffer.size = desc.size;
	glbuffer.persistentPtr = persistentPtr;

	gl_set_debug_name(glbuffer.target, id, -1, desc.name);

	return g_buffers.insert(glbuffer);
}

bx::handle_id bx::gfx_create_buffer(const gfx_buffer_desc_t& desc) bx_noexcept
{
	bx_profile(bx);

#ifdef BXL_GFX_OPENGLES
	return gl33_create_buffer(desc);
#else
	return gl46_create_buffer(desc);
#endif
}

void bx::gfx_destroy_buffer(const handle_id handle) bx_noexcept
{
	bx_profile(bx);

	auto glbuff = g_buffers.get(handle);
	if (!glbuff) return;

	glDeleteBuffers(1, &glbuff->id);

	g_buffers.remove(handle);
}

static u8* gl33_map_buffer(const bx::handle_id handle, const u64 offset, const u64 size)
{
	bx_profile(bx);

	auto glbuffer = g_buffers.get(handle);
	if (!glbuffer) return nullptr;

	// Persistent mapped buffer path
	if (glbuffer->persistentPtr)
		return static_cast<u8*>(glbuffer->persistentPtr) + offset;

	glBindBuffer(glbuffer->target, glbuffer->id);

	GLbitfield access = 0;
	switch (glbuffer->mem_usage)
	{
	case bx::gfx_memory_usage_t::CPU_TO_GPU:
		access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
		break;
	case bx::gfx_memory_usage_t::GPU_TO_CPU:
		access = GL_MAP_READ_BIT;
		break;
	case bx::gfx_memory_usage_t::GPU_ONLY:
	default:
		bx_loge(bx, "Attempted to map GPU-only buffer '{}' (ID: {}, Target: 0x{:X}, Size: {} bytes). Mapping is not allowed.",
			glbuffer->name ? glbuffer->name : "unnamed", glbuffer->id, glbuffer->target, glbuffer->size);
		glBindBuffer(glbuffer->target, 0);
		return nullptr;
	}

	void* ptr = glMapBufferRange(
		glbuffer->target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), access);
	glBindBuffer(glbuffer->target, 0);
	return reinterpret_cast<u8*>(ptr);
}

static u8* gl46_map_buffer(const bx::handle_id handle, const u64 offset, const u64 size)
{
	bx_profile(bx);

	auto glbuffer = g_buffers.get(handle);
	if (!glbuffer) return nullptr;

	GLbitfield access = 0;
	switch (glbuffer->mem_usage)
	{
	case bx::gfx_memory_usage_t::CPU_TO_GPU:
		access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;
		break;

	case bx::gfx_memory_usage_t::GPU_TO_CPU:
		access = GL_MAP_READ_BIT;
		break;

	case bx::gfx_memory_usage_t::GPU_ONLY:
	default:
		bx_loge(bx, "Attempted to map GPU-only buffer '{}' (ID: {}, Target: 0x{:X}, Size: {} bytes). Mapping is not allowed.",
			glbuffer->name ? glbuffer->name : "unnamed", glbuffer->id, glbuffer->target, glbuffer->size);
		return nullptr;
	}

	vptr ptr = glMapNamedBufferRange(
		glbuffer->id, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), access);
	return static_cast<u8*>(ptr);
}

u8* bx::gfx_map_buffer(const handle_id handle, const u64 offset, const u64 size) bx_noexcept
{
	bx_profile(bx);

#ifdef BXL_GFX_OPENGLES
	return gl33_map_buffer(handle, offset, size);
#else
	return gl46_map_buffer(handle, offset, size);
#endif
}

static void gl33_unmap_buffer(const bx::handle_id handle)
{
	bx_profile(bx);

	auto glbuffer = g_buffers.get(handle);
	if (!glbuffer || glbuffer->persistentPtr)
		return;

	glBindBuffer(glbuffer->target, glbuffer->id);
	glUnmapBuffer(glbuffer->target);
	glBindBuffer(glbuffer->target, 0);
}

static void gl46_unmap_buffer(const bx::handle_id handle)
{
	bx_profile(bx);

	auto glbuffer = g_buffers.get(handle);
	if (!glbuffer || glbuffer->persistentPtr)
		return;

	glUnmapNamedBuffer(glbuffer->id);
}

void bx::gfx_unmap_buffer(const handle_id handle) bx_noexcept
{
	bx_profile(bx);

#ifdef BXL_GFX_OPENGLES
	return gl33_unmap_buffer(handle);
#else
	return gl46_unmap_buffer(handle);
#endif
}

static void gl33_update_buffer(const bx::handle_id handle, const u64 dst_offset, cvptr src, const u64 size)
{
	bx_profile(bx);

	auto glbuffer = g_buffers.get(handle);
	if (!glbuffer) return;

	if (glbuffer->persistentPtr)
	{
		std::memcpy(static_cast<u8*>(glbuffer->persistentPtr) + dst_offset, src, size);
		return;
	}

	glBindBuffer(glbuffer->target, glbuffer->id);
	glBufferSubData(
		glbuffer->target, static_cast<GLintptr>(dst_offset), static_cast<GLsizeiptr>(size), src);
	glBindBuffer(glbuffer->target, 0);
}

static void gl46_update_buffer(const bx::handle_id handle, const u64 dst_offset, cvptr src, const u64 size)
{
	bx_profile(bx);

	auto glbuffer = g_buffers.get(handle);
	if (!glbuffer) return;

	if (glbuffer->persistentPtr)
	{
		std::memcpy(static_cast<u8*>(glbuffer->persistentPtr) + dst_offset, src, size);
		return;
	}

	glNamedBufferSubData(
		glbuffer->id, static_cast<GLintptr>(dst_offset), static_cast<GLsizeiptr>(size), src);
}

void bx::gfx_update_buffer(const handle_id handle, const u64 dst_offset, cvptr src, const u64 size) bx_noexcept
{
	bx_profile(bx);

#ifdef BXL_GFX_OPENGLES
	return gl33_update_buffer(handle, dst_offset, src, size);
#else
	return gl46_update_buffer(handle, dst_offset, src, size);
#endif
}

static GLenum gl_target_from_type(bx::gfx_texture_type_t type)
{
	bx_profile(bx);

	switch (type)
	{
	case bx::gfx_texture_type_t::TEX2D: return GL_TEXTURE_2D;
	case bx::gfx_texture_type_t::TEX3D: return GL_TEXTURE_3D;
	case bx::gfx_texture_type_t::TEX_CUBE: return GL_TEXTURE_CUBE_MAP;
	default:
		return GL_TEXTURE_2D;
	}
}

static GLenum gl_format_from_texture_format(bx::gfx_texture_format_t fmt, GLenum& internal, GLenum& type)
{
	bx_profile(bx);

	switch (fmt)
	{
	case bx::gfx_texture_format_t::R8U_NORM:
		internal = GL_R8;
		type = GL_UNSIGNED_BYTE;
		return GL_RED;
	case bx::gfx_texture_format_t::RG8U_NORM:
		internal = GL_RG8;
		type = GL_UNSIGNED_BYTE;
		return GL_RG;
	case bx::gfx_texture_format_t::RGBA8U_NORM:
		internal = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		return GL_RGBA;
	case bx::gfx_texture_format_t::RGBA16F:
		internal = GL_RGBA16F;
		type = GL_HALF_FLOAT;
		return GL_RGBA;
	case bx::gfx_texture_format_t::DEPTH24_STENCIL8:
		internal = GL_DEPTH24_STENCIL8;
		type = GL_UNSIGNED_INT_24_8;
		return GL_DEPTH_STENCIL;
	default:
		internal = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		return GL_RGBA;
	}
}

bx::handle_id bx::gfx_create_texture(const gfx_texture_desc_t& desc) bx_noexcept
{
	bx_profile(bx);

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

	gl_texture_t gltexture{};
	gltexture.id = tex;
	gltexture.type = desc.type;
	gltexture.format = desc.format;
	gltexture.width = desc.width;
	gltexture.height = desc.height;
	gltexture.depth = desc.depth;
	gltexture.mip_levels = desc.mip_levels;

	return g_textures.insert(gltexture);
}

void bx::gfx_destroy_texture(const handle_id handle) bx_noexcept
{
	bx_profile(bx);

	auto gltex = g_textures.get(handle);
	if (!gltex) return;

	glDeleteTextures(1, &gltex->id);
	
	g_textures.remove(handle);
}

void bx::gfx_upload_texture_data(const handle_id texture, const u8* data, const u32 region_count, const gfx_texture_region_t* regions) bx_noexcept
{
	bx_profile(bx);

	auto gltexture = g_textures.get(texture);
	if (!gltexture || !data || !regions)
		return;

	const GLenum target = gl_target_from_type(gltexture->type);

	GLenum internal = 0, pixel_fmt = 0, type = 0;
	pixel_fmt = gl_format_from_texture_format(gltexture->format, internal, type);

	glBindTexture(target, gltexture->id);
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
				static_cast<GLsizei>(r.width),
				static_cast<GLsizei>(r.height),
				pixel_fmt, type, src
			);
			break;
		case GL_TEXTURE_CUBE_MAP:
			// Cube faces would need to be specified as regions (you can expand later)
			glTexSubImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X, r.mip_level,
				r.x, r.y,
				static_cast<GLsizei>(r.width),
				static_cast<GLsizei>(r.height),
				pixel_fmt, type, src
			);
			break;
		case GL_TEXTURE_3D:
			glTexSubImage3D(
				target, r.mip_level,
				r.x, r.y, 0,
				static_cast<GLsizei>(r.width),
				static_cast<GLsizei>(r.height),
				static_cast<GLsizei>(gltexture->depth),
				pixel_fmt, type, src
			);
			break;
		default:
			break;
		}
	}

	glBindTexture(target, 0);
}

bx::handle_id bx::gfx_create_framebuffer(const gfx_framebuffer_desc_t& desc) bx_noexcept
{
	bx_profile(bx);

	GLuint fbo = 0;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	for (u32 i = 0; i < desc.color_textures.size; ++i)
	{
		auto gltex = g_textures.get(desc.color_textures[i]);
		if (!gltex)
			continue;

		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
			GL_TEXTURE_2D, gltex->id, 0);
	}

	if (desc.depth_texture)
	{
		auto gldepthtex = g_textures.get(desc.depth_texture);
		if (gldepthtex)
		{
			const GLenum attachment = (gldepthtex->format == gfx_texture_format_t::DEPTH24_STENCIL8)
				? GL_DEPTH_STENCIL_ATTACHMENT
				: GL_DEPTH_ATTACHMENT;

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, gldepthtex->id, 0);
		}
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		bx_loge(bx, "Framebuffer incomplete");
		glDeleteFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return invalid_handle;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	gl_framebuffer_t glfb{};
	glfb.id = fbo;
	glfb.width = desc.width;
	glfb.height = desc.height;
	glfb.color_textures.assign(desc.color_textures.data, desc.color_textures.data + desc.color_textures.size);
	glfb.depth_texture = desc.depth_texture;
	glfb.render_pass = desc.render_pass;

	return g_framebuffers.insert(glfb);
}

void bx::gfx_destroy_framebuffer(handle_id fb) bx_noexcept
{
	bx_profile(bx);

	auto glfb = g_framebuffers.get(fb);
	if (!glfb) return;

	glDeleteFramebuffers(1, &glfb->id);

	g_framebuffers.remove(fb);
}

bx::handle_id bx::gfx_default_framebuffer() bx_noexcept
{
	bx_profile(bx);

	return handle_id{ 0 };
}

static void gl_vattrib_info(bx::gfx_attribute_format_t fmt, GLenum& type, GLint& sizebytes)
{
	bx_profile(bx);

	switch (fmt)
	{
	case bx::gfx_attribute_format_t::FLOAT32:
		type = GL_FLOAT;
		sizebytes = sizeof(f32);
		break;
		//case gfx_attribute_format_t::UINT8_4_NORM:
		//	type = GL_UNSIGNED_BYTE;
		//	normalized = GL_TRUE;
		//	break;
	default:
		break;
	}
}

static bx::handle_id gl33_create_pipeline(const bx::gfx_pipeline_desc_t& desc)
{
	bx_profile(bx);

	if (!desc.shaders)
	{
		bx_loge(bx, "gfx_create_pipeline: no shaders provided");
		return bx::invalid_handle;
	}

	const GLuint program = glCreateProgram();
	for (const auto sh : desc.shaders)
	{
		const auto shader = bx::handle_t{ sh }.get_data<GLuint>();
		glAttachShader(program, shader);
	}
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
		bx_loge(bx, log.c_str());
		glDeleteProgram(program);
		return bx::invalid_handle;
	}

	for (const auto sh : desc.shaders)
	{
		const auto shader = bx::handle_t{ sh }.get_data<GLuint>();
		glDetachShader(program, shader);
	}

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLsizei stride = 0;
	for (u32 i = 0; i < desc.input_layout.attributes.size; ++i)
	{
		const auto& attr = desc.input_layout.attributes[i];
		GLenum type = GL_FLOAT;
		GLint sizebytes = 0;
		GLint count = attr.count;
		gl_vattrib_info(attr.format, type, sizebytes);
		stride += sizebytes * count;
}

	GLuint relative_offset = 0;
	for (u32 i = 0; i < desc.input_layout.attributes.size; ++i)
	{
		const auto& attr = desc.input_layout.attributes[i];

		GLuint attrib_index = attr.location;
		GLuint binding_index = attr.binding;
		GLint count = attr.count;
		GLenum type = GL_FLOAT;
		GLint sizebytes = 0;
		gl_vattrib_info(attr.format, type, sizebytes);

		GLboolean normalized = attr.normalized;
		if (attr.offset != 0)
			relative_offset = attr.offset;

		// Bind a dummy buffer to the binding point for now
		// Actual buffer will be bound at draw time
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(attrib_index);
		glVertexAttribFormat(attrib_index, count, type, normalized, relative_offset);
		glVertexAttribBinding(attrib_index, binding_index);
		//glVertexAttribDivisor(attrib_index, attr.input_rate_per_vertex_or_instance);

		relative_offset += sizebytes * count;
	}

	glBindVertexArray(0);

	gl_set_debug_name(GL_PROGRAM, program, -1, desc.name);
	gl_set_debug_name(GL_VERTEX_ARRAY, vao, -1, desc.name);

	gl_pipeline_t glpipeline{};
	glpipeline.pipeline = program;
	glpipeline.vao = vao;
	glpipeline.topology = desc.topology;
	glpipeline.raster = desc.raster;

	if (desc.color_attachments)
	{
		glpipeline.blend_attachments.assign(
			desc.color_attachments.data,
			desc.color_attachments.data + desc.color_attachments.size);
	}

	return g_pipelines.insert(glpipeline);
}

static bx::handle_id gl46_create_pipeline(const bx::gfx_pipeline_desc_t& desc)
{
	bx_profile(bx);

	if (!desc.shaders)
	{
		bx_loge(bx, "gfx_create_pipeline: no shaders provided");
		return 0;
	}

	GLuint pipeline = 0;
	glCreateProgramPipelines(1, &pipeline);

	for (const auto sh : desc.shaders)
	{
		const auto glsh = g_shaders.get(sh);
		if (!glsh) continue;

		GLbitfield stagebit = 0;
		switch (glsh->stage)
		{
		case GL_VERTEX_SHADER:          stagebit = GL_VERTEX_SHADER_BIT; break;
		case GL_FRAGMENT_SHADER:        stagebit = GL_FRAGMENT_SHADER_BIT; break;
		case GL_GEOMETRY_SHADER:        stagebit = GL_GEOMETRY_SHADER_BIT; break;
		case GL_TESS_CONTROL_SHADER:    stagebit = GL_TESS_CONTROL_SHADER_BIT; break;
		case GL_TESS_EVALUATION_SHADER: stagebit = GL_TESS_EVALUATION_SHADER_BIT; break;
		case GL_COMPUTE_SHADER:         stagebit = GL_COMPUTE_SHADER_BIT; break;
		default:
			bx_loge(bx, "gfx_create_pipeline: unknown shader stage");
			glDeleteProgramPipelines(1, &pipeline);
			return 0;
		}

		glUseProgramStages(pipeline, stagebit, glsh->shader);
	}

	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);

	GLsizei stride = 0;
	for (u32 i = 0; i < desc.input_layout.attributes.size; ++i)
	{
		const auto& attr = desc.input_layout.attributes[i];
		GLenum type = GL_FLOAT;
		GLint sizebytes = 0;
		gl_vattrib_info(attr.format, type, sizebytes);
		stride += sizebytes * attr.count;
	}

	GLuint relative_offset = 0;
	for (u32 i = 0; i < desc.input_layout.attributes.size; ++i)
	{
		const auto& attr = desc.input_layout.attributes[i];

		GLuint attrib_index = attr.location;
		GLuint binding_index = attr.binding;
		GLint count = attr.count;
		GLenum type = GL_FLOAT;
		GLint sizebytes = 0;
		gl_vattrib_info(attr.format, type, sizebytes);

		GLboolean normalized = attr.normalized;
		if (attr.offset != 0)
			relative_offset = attr.offset;

		// Bind a dummy buffer to the binding point for now
		// Actual buffer will be bound at draw time
		glVertexArrayVertexBuffer(vao, binding_index, 0, 0, stride);

		glEnableVertexArrayAttrib(vao, attrib_index);
		glVertexArrayAttribFormat(vao, attrib_index, count, type, normalized, relative_offset);
		glVertexArrayAttribBinding(vao, attrib_index, binding_index);
		glVertexArrayBindingDivisor(vao, binding_index, attr.input_rate_per_vertex_or_instance);

		relative_offset += sizebytes * count;
	}

	gl_set_debug_name(GL_VERTEX_ARRAY, vao, -1, desc.name);

	gl_pipeline_t glpipeline{};
	glpipeline.pipeline = pipeline;
	glpipeline.vao = vao;
	glpipeline.topology = desc.topology;
	glpipeline.raster = desc.raster;

	if (desc.color_attachments)
	{
		glpipeline.blend_attachments.assign(
			desc.color_attachments.data,
			desc.color_attachments.data + desc.color_attachments.size);
	}

	return g_pipelines.insert(glpipeline);
}

bx::handle_id bx::gfx_create_pipeline(const gfx_pipeline_desc_t& desc) bx_noexcept
{
	bx_profile(bx);

#ifdef BXL_GFX_OPENGLES
	return gl33_create_pipeline(desc);
#else
	return gl46_create_pipeline(desc);
#endif
}

void bx::gfx_destroy_pipeline(const handle_id handle) bx_noexcept
{
	bx_profile(bx);

	auto glpipeline = g_pipelines.get(handle);
	if (!glpipeline) return;

#ifdef BXL_GFX_OPENGLES
	glDeleteProgram(glpipeline->pipeline);
#else
	glDeleteProgramPipelines(1, &glpipeline->pipeline);
#endif

	if (glpipeline->vao)
		glDeleteVertexArrays(1, &glpipeline->vao);

	g_pipelines.remove(handle);
}

bx::handle_id bx::gfx_create_resource_set(const gfx_resource_set_desc_t& desc) bx_noexcept
{
	bx_profile(bx);

	return handle_id{0};
}

//void bx::gfx_update_resource_set(handle_id set_handle, u32 binding_count, const gfx_resource_binding_t* bindings) bx_noexcept {}

void bx::gfx_destroy_resource_set(handle_id set_handle) bx_noexcept
{
	bx_profile(bx);
}

void bx::gfx_clear_rt(handle_id rt, f32 cv[4]) bx_noexcept
{
	bx_profile(bx);

	glClearColor(cv[0], cv[1], cv[2], cv[3]);
	glClear(GL_COLOR_BUFFER_BIT);
}

void bx::gfx_clear_ds(handle_id ds)
{
	bx_profile(bx);

	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void bx::gfx_bind_pipeline(handle_id cb, handle_id pipeline) bx_noexcept
{
	bx_profile(bx);

	auto glpipeline = g_pipelines.get(pipeline);
	if (!glpipeline) return;

	g_current_pipeline = pipeline;

#ifdef BXL_GFX_OPENGLES
	glUseProgram(glpipeline->pipeline);
#else
	glBindProgramPipeline(glpipeline->pipeline);
#endif

	if (glpipeline->vao)
		glBindVertexArray(glpipeline->vao);
	else
		glBindVertexArray(0);

	if (glpipeline->raster.cull_enable)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	if (glpipeline->raster.depth_test)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	glDepthMask(glpipeline->raster.depth_write ? GL_TRUE : GL_FALSE);

	glDisable(GL_BLEND);
	for (auto& a : glpipeline->blend_attachments)
	{
		if (a.blend_enable)
		{
			glEnable(GL_BLEND);
			break;
		}
	}
}

void bx::gfx_bind_vertex_buffers(handle_id cmd, u32 first_binding, u32 binding_count, const handle_id* vertex_buffers, const u64* offsets) bx_noexcept
{
	bx_profile(bx);

#ifdef BXL_GFX_OPENGLES
	for (u32 i = 0; i < binding_count; ++i)
	{
		const auto glbuff = g_buffers.get(vertex_buffers[i]);
		if (!glbuff) continue;

		const GLintptr offset = static_cast<GLintptr>(offsets ? offsets[i] : 0);
		// Stride is baked into the vertex input description at pipeline creation time.
		//const GLsizei stride = get_bound_vbo_stride(first_binding + i);
		//glBindVertexBuffer(first_binding + i, vbo, offset, stride);
		const GLuint bindingindex = first_binding + i;
		glBindVertexBuffer(bindingindex, glbuff->id, offset, 16);
	}
#else
	static GLuint tmp_buffers[MAX_BOUND_VERTEX_BUFFERS]{};
	static GLintptr tmp_offset[MAX_BOUND_VERTEX_BUFFERS]{};
	static GLsizei tmp_strides[MAX_BOUND_VERTEX_BUFFERS]{};
	for (i32 i = 0; i < binding_count; i++)
	{
		const auto glbuff = g_buffers.get(vertex_buffers[i]);
		if (!glbuff) continue;
		tmp_buffers[i] = glbuff->id;
		tmp_offset[i] = 0;
		tmp_strides[i] = 16;
	}
	glBindVertexBuffers(first_binding, binding_count, tmp_buffers, tmp_offset, tmp_strides);
#endif
}

void bx::gfx_bind_index_buffer(handle_id cb, handle_id index_buffer, u32 index_type) bx_noexcept
{
	bx_profile(bx);
}

void bx::gfx_bind_resource_set(handle_id cb, handle_id pipeline_handle, handle_id set_handle, u32 set_index) bx_noexcept
{
	bx_profile(bx);
}

static GLenum gl_enum_from_topology(bx::gfx_topology_t t)
{
	bx_profile(bx);

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

void bx::gfx_draw(handle_id cb, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) bx_noexcept
{
	bx_profile(bx);

	// pick topology from current pipeline
	GLenum mode = GL_TRIANGLES;
	const auto glpipeline = g_pipelines.get(g_current_pipeline);
	if (glpipeline)
		mode = gl_enum_from_topology(glpipeline->topology);

	if (instance_count <= 1)
	{
		GLint first = static_cast<GLint>(first_vertex);
		GLsizei count = static_cast<GLsizei>(vertex_count);
		glDrawArrays(mode, first, count);
	}
	else
	{
		glDrawArraysInstanced(
			mode, static_cast<GLint>(first_vertex), static_cast<GLsizei>(vertex_count),
			static_cast<GLsizei>(instance_count));
	}
}

void bx::gfx_draw_indexed(handle_id cb, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset) bx_noexcept
{
	bx_profile(bx);
}

void bx::gfx_copy_buffer(handle_id cb, handle_id src, handle_id dst, u64 src_offset, u64 dst_offset, u64 size) bx_noexcept
{
	bx_profile(bx);
}

void bx::gfx_dispatch(handle_id cb, u32 x, u32 y, u32 z) bx_noexcept
{
	bx_profile(bx);
}

void bx::gfx_submit(handle_id cb) bx_noexcept
{
	bx_profile(bx);
}

void bx::gfx_wait_idle() bx_noexcept
{
	bx_profile(bx);
}

void bx::gfx_pipeline_barrier(handle_id cb, const gfx_memory_barrier_t& barrier) bx_noexcept
{
	bx_profile(bx);
}