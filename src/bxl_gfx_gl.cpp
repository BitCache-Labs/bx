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
	std::vector<bx::gfx_vertex_attribute_t> attributes{};
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

struct gl_features_t
{
	// --- Modern API architecture ---
	bool sso{ false };                   // Separable Shader Objects
	bool dsa{ false };                   // Direct State Access

	// --- Buffers & GPU memory ---
	bool buffer_storage{ false };        // ARB_buffer_storage (persistent/coherent mapping)
	bool persistent_mapping{ false };    // MAP_PERSISTENT_BIT + MAP_COHERENT_BIT
	bool ssbo{ false };                  // Shader Storage Buffer Objects (ARB_shader_storage_buffer_object)
	bool atomic_counters{ false };       // ARB_shader_atomic_counters
	bool shader_image_load_store{ false }; // ARB_shader_image_load_store

	// --- Compute / GPU-driven ---
	bool compute_shader{ false };        // ARB_compute_shader, GL 4.3+
	bool multi_draw{ false };			 // ARB_multi_draw
	bool draw_indirect{ false };         // ARB_draw_indirect
	bool multi_draw_indirect{ false };   // ARB_multi_draw_indirect
	bool indirect_count{ false };        // ARB_indirect_parameters (DrawIndirectCountARB)

	// --- Textures ---
	bool texture_storage{ false };       // ARB_texture_storage
	bool texture_view{ false };          // ARB_texture_view
	bool bindless_textures{ false };     // NV_bindless_texture
	bool sparse_texture{ false };        // ARB_sparse_texture

	// --- Vertex / pipeline state ---
	bool vertex_attrib_binding{ false }; // ARB_vertex_attrib_binding (core 4.3)
	bool vao{ false };                   // Vertex Array Objects available
	bool multi_bind{ false };            // ARB_multi_bind (bind many images/buffers/etc at once)

	// --- Shaders ---
	bool tessellation_shader{ false };   // ARB_tessellation_shader
	bool geometry_shader{ false };       // ARB_geometry_shader4
	bool shader_texture_lod{ false };    // ARB_shader_texture_lod (important for PBR)

	// --- Blending / framebuffer ---
	bool advanced_blend{ false };        // KHR_blend_equation_advanced
	bool framebuffer_no_attachments{ false }; // ARB_framebuffer_no_attachments

	// --- Debug / profiling ---
	bool debug_output{ false };          // KHR_debug / ARB_debug_output
	bool timer_query{ false };           // ARB_timer_query, GPU timestamps

	// --- Viewport / misc ---
	bool clip_control{ false };          // ARB_clip_control (Vulkan-style depth)
	bool viewport_array{ false };        // ARB_viewport_array

	// --- Compression formats ---
	bool tex_compression_bptc{ false };  // BC7 / BC6H
	bool tex_compression_s3tc{ false };  // BC1–BC5 (DXT)
	bool tex_compression_astc{ false };  // ASTC
	bool tex_compression_etc2{ false };  // ETC2/EAC
};

static gl_features_t g_features;

static void gl_print_info()
{
	bx::log(bx::log_t::DEBUG, "OpenGL information:");
	bx::logf(bx::log_t::DEBUG, "\tgl version:		{}", (cstring)glGetString(GL_VERSION));
	bx::logf(bx::log_t::DEBUG, "\tglsl version:		{}", (cstring)glGetString(GL_SHADING_LANGUAGE_VERSION));
	bx::logf(bx::log_t::DEBUG, "\tvendor:			{}", (cstring)glGetString(GL_VENDOR));
	bx::logf(bx::log_t::DEBUG, "\trenderer:			{}", (cstring)glGetString(GL_RENDERER));

	GLint value = 0;

	bx::log(bx::log_t::DEBUG, "OpenGL limits:");

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Texture Size:					{}", value);

	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax 3D Texture Size:				{}", value);

	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Array Texture Layers:			{}", value);

	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Vertex Attributes:			{}", value);

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Vertex Uniform Components:	{}", value);

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Vertex Uniform Blocks:		{}", value);

	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Fragment Uniform Components:	{}", value);

	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Fragment Uniform Blocks:		{}", value);

	glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Combined Uniform Blocks:		{}", value);

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Texture Image Units:			{}", value);

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Vertex Texture Image Units:	{}", value);

	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Color Attachments:			{}", value);

	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Draw Buffers:					{}", value);

	glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Framebuffer Width:			{}", value);

	glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Framebuffer Height:			{}", value);

	glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Framebuffer Samples:			{}", value);

	glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Vertex Buffer Bindings:		{}", value);

	glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Uniform Buffer Bindings:		{}", value);

	glGetIntegerv(GL_MAX_ELEMENT_INDEX, &value);
	bx::logf(bx::log_t::DEBUG, "\tMax Element Index:				{}", value);

	GLint count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	bx::logf(bx::log_t::DEBUG, "OpenGL extensions ({}):", count);

	for (GLint i = 0; i < count; ++i)
	{
		bx::logf(bx::log_t::DEBUG, "\t{}", (cstring)glGetStringi(GL_EXTENSIONS, i));
	}
}

static bool gl_has_ext(cstring ext)
{
	GLint count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (GLint i = 0; i < count; i++)
	{
		cstring e = (cstring)glGetStringi(GL_EXTENSIONS, i);
		if (e && strcmp(e, ext) == 0)
			return true;
	}
	return false;
}

static void gl_check_features()
{
	// -------------------------------------------------------------------------
	// Version check helper
	// -------------------------------------------------------------------------
	int major = 0, minor = 0;
	{
		const char* ver = (const char*)glGetString(GL_VERSION);
		if (ver)
			sscanf(ver, "%d.%d", &major, &minor);
	}

	// -------------------------------------------------------------------------
	// Separable Shader Objects (SSO)
	// -------------------------------------------------------------------------
#if GL_ARB_separate_shader_objects
	g_features.sso =
		(glCreateShaderProgramv &&
			glProgramParameteri &&
			(glCreateProgramPipelines || glGenProgramPipelines || glGenProgramPipelinesEXT) &&
			(glDeleteProgramPipelines || glDeleteProgramPipelinesEXT) &&
			(glBindProgramPipeline || glBindProgramPipelineEXT) &&
			(glUseProgramStages || glUseProgramStagesEXT) &&
			(glActiveShaderProgram || glActiveShaderProgramEXT) &&
			(glGetProgramPipelineiv || glGetProgramPipelineivEXT) &&
			(glValidateProgramPipeline || glValidateProgramPipelineEXT) &&
			(glGetProgramPipelineInfoLog || glGetProgramPipelineInfoLogEXT)) &&
		((major > 4 || (major == 4 && minor >= 1)) ||
			gl_has_ext("GL_ARB_separate_shader_objects"));
#endif

	// -------------------------------------------------------------------------
	// Direct State Access (DSA)
	// -------------------------------------------------------------------------
#if GL_ARB_direct_state_access || GL_EXT_direct_state_access
	g_features.dsa =
		((glCreateBuffers && glNamedBufferStorage &&
			glCreateTextures && glTextureStorage2D &&
			glCreateVertexArrays && glVertexArrayVertexBuffer &&
			glCreateFramebuffers && glNamedFramebufferTexture &&
			glCreateSamplers && glSamplerParameterf &&
			glProgramUniform1i) ||
			glTextureStorage2DEXT) &&
		((major > 4 || (major == 4 && minor >= 5)) ||
			gl_has_ext("GL_ARB_direct_state_access") ||
			gl_has_ext("GL_EXT_direct_state_access"));
#endif

	// -------------------------------------------------------------------------
	// Buffers and memory
	// -------------------------------------------------------------------------
#if GL_ARB_buffer_storage
	g_features.buffer_storage =
		glBufferStorage &&
		(gl_has_ext("GL_ARB_buffer_storage") || gl_has_ext("GL_EXT_buffer_storage"));
#endif

	g_features.persistent_mapping =
		g_features.buffer_storage &&
		(GL_MAP_PERSISTENT_BIT && GL_MAP_COHERENT_BIT);

#if GL_ARB_shader_storage_buffer_object
	g_features.ssbo =
		(glBindBufferBase || glBindBufferRange) &&
		(gl_has_ext("GL_ARB_shader_storage_buffer_object") ||
			(major > 4 || (major == 4 && minor >= 3)));
#endif

#if GL_ARB_shader_atomic_counters
	g_features.atomic_counters =
		glBindBufferBase &&
		(gl_has_ext("GL_ARB_shader_atomic_counters") ||
			(major > 4 || (major == 4 && minor >= 2)));
#endif

#if GL_ARB_shader_image_load_store
	g_features.shader_image_load_store =
		glBindImageTexture &&
		(gl_has_ext("GL_ARB_shader_image_load_store") ||
			gl_has_ext("GL_EXT_shader_image_load_store"));
#endif

	// -------------------------------------------------------------------------
	// Compute shader
	// -------------------------------------------------------------------------
#if GL_ARB_compute_shader
	g_features.compute_shader =
		glDispatchCompute &&
		(gl_has_ext("GL_ARB_compute_shader") ||
			(major > 4 || (major == 4 && minor >= 3)));
#endif

	// -------------------------------------------------------------------------
	// Multi draw
	// -------------------------------------------------------------------------
#if GL_ARB_multi_draw || GL_EXT_multi_draw_arrays || GL_NV_multi_draw
	g_features.multi_draw =
		(glMultiDrawArrays || glMultiDrawElements ||
			glMultiDrawArraysEXT || glMultiDrawElementsEXT) &&
		(gl_has_ext("GL_ARB_multi_draw") ||
			gl_has_ext("GL_EXT_multi_draw_arrays") ||
			gl_has_ext("GL_NV_multi_draw"));
#endif

	// -------------------------------------------------------------------------
	// Indirect draw
	// -------------------------------------------------------------------------

#if GL_ARB_draw_indirect || GL_EXT_draw_indirect
	g_features.draw_indirect =
		(glDrawArraysIndirect || glDrawElementsIndirect) &&
		(gl_has_ext("GL_ARB_draw_indirect") ||
			gl_has_ext("GL_EXT_draw_indirect") ||
			(major > 4 || (major == 4 && minor >= 0)) ||
			(major > 3 || (major == 3 && minor >= 1)));
#endif

#if GL_ARB_multi_draw_indirect || GL_EXT_multi_draw_indirect
	g_features.multi_draw_indirect =
		(glMultiDrawArraysIndirect || glMultiDrawElementsIndirect ||
			glMultiDrawArraysIndirectEXT || glMultiDrawElementsIndirectEXT) &&
		(gl_has_ext("GL_ARB_multi_draw_indirect") ||
			gl_has_ext("GL_EXT_multi_draw_indirect"));
#endif

#if GL_ARB_indirect_parameters || GL_NV_command_list
	g_features.indirect_count =
		(glMultiDrawArraysIndirectCountARB || glMultiDrawElementsIndirectCountARB) &&
		(gl_has_ext("GL_ARB_indirect_parameters") ||
			gl_has_ext("GL_NV_command_list"));
#endif

	// -------------------------------------------------------------------------
	// Textures
	// -------------------------------------------------------------------------
#if GL_ARB_texture_storage
	g_features.texture_storage =
		(glTextureStorage1D || glTextureStorage2D || glTextureStorage3D) &&
		(gl_has_ext("GL_ARB_texture_storage") ||
			gl_has_ext("GL_EXT_texture_storage") ||
			(major > 4 || (major == 4 && minor >= 2)));
#endif

#if GL_ARB_texture_view
	g_features.texture_view =
		glTextureView &&
		(gl_has_ext("GL_ARB_texture_view") ||
			gl_has_ext("GL_EXT_texture_view") ||
			(major > 4 || (major == 4 && minor >= 3)));
#endif

#if GL_ARB_bindless_texture || GL_NV_bindless_texture
	g_features.bindless_textures =
		(glGetTextureHandleARB || glMakeTextureHandleResidentARB) &&
		(gl_has_ext("GL_ARB_bindless_texture") ||
			gl_has_ext("GL_NV_bindless_texture"));
#endif

#if GL_ARB_sparse_texture
	g_features.sparse_texture =
		glTexPageCommitmentARB &&
		(gl_has_ext("GL_ARB_sparse_texture") ||
			gl_has_ext("GL_EXT_sparse_texture"));
#endif

	// -------------------------------------------------------------------------
	// Vertex / pipeline
	// -------------------------------------------------------------------------
#if GL_ARB_vertex_attrib_binding
	g_features.vertex_attrib_binding =
		glVertexArrayVertexBuffer &&
		(gl_has_ext("GL_ARB_vertex_attrib_binding") ||
			gl_has_ext("GL_EXT_vertex_attrib_binding") ||
			(major > 4 || (major == 4 && minor >= 3)));
#endif

#if GL_ARB_vertex_array_object
	g_features.vao =
		glBindVertexArray &&
		(gl_has_ext("GL_ARB_vertex_array_object") ||
			gl_has_ext("GL_APPLE_vertex_array_object") || // Apple uses a different name
			(major >= 3));
#endif

#if GL_ARB_multi_bind
	g_features.multi_bind =
		(glBindTextures || glBindSamplers || glBindBuffersBase) &&
		(gl_has_ext("GL_ARB_multi_bind") ||
			gl_has_ext("GL_EXT_multi_bind"));
#endif

	// -------------------------------------------------------------------------
	// Shader stages
	// -------------------------------------------------------------------------
#if GL_ARB_tessellation_shader
	g_features.tessellation_shader =
		glPatchParameteri &&
		(gl_has_ext("GL_ARB_tessellation_shader") ||
			gl_has_ext("GL_EXT_tessellation_shader") ||
			(major > 4 || (major == 4 && minor >= 0)));
#endif

#if GL_ARB_geometry_shader4
	g_features.geometry_shader =
		glProgramParameteri &&
		(gl_has_ext("GL_ARB_geometry_shader4") ||
			gl_has_ext("GL_EXT_geometry_shader4") ||
			(major > 3 || (major == 3 && minor >= 2)));
#endif

#if GL_ARB_shader_texture_lod
	g_features.shader_texture_lod =
		gl_has_ext("GL_ARB_shader_texture_lod") ||
		gl_has_ext("GL_EXT_shader_texture_lod");
#endif

	// -------------------------------------------------------------------------
	// Blending / framebuffer
	// -------------------------------------------------------------------------
#if GL_KHR_blend_equation_advanced
	g_features.advanced_blend =
		glBlendBarrierKHR &&
		(gl_has_ext("GL_KHR_blend_equation_advanced") ||
			gl_has_ext("GL_NV_blend_equation_advanced"));
#endif

#if GL_ARB_framebuffer_no_attachments
	g_features.framebuffer_no_attachments =
		glFramebufferParameteri &&
		(gl_has_ext("GL_ARB_framebuffer_no_attachments") ||
			gl_has_ext("GL_EXT_framebuffer_no_attachments"));
#endif

	// -------------------------------------------------------------------------
	// Debug / profiling
	// -------------------------------------------------------------------------
#if GL_ARB_debug_output || GL_KHR_debug
	g_features.debug_output =
		(glDebugMessageCallback || glDebugMessageControl) &&
		(gl_has_ext("GL_ARB_debug_output") || gl_has_ext("GL_KHR_debug"));
#endif

#if GL_ARB_timer_query || GL_EXT_timer_query
	g_features.timer_query =
		(glQueryCounter || glQueryCounterEXT) &&
		(gl_has_ext("GL_ARB_timer_query") || gl_has_ext("GL_EXT_timer_query"));
#endif

	// -------------------------------------------------------------------------
	// Viewport / clip
	// -------------------------------------------------------------------------
#if GL_ARB_clip_control
	g_features.clip_control =
		(glClipControl || glClipControlEXT) &&
		(gl_has_ext("GL_ARB_clip_control") ||
			gl_has_ext("GL_EXT_clip_control") ||
			(major > 4 || (major == 4 && minor >= 5)));
#endif

#if GL_ARB_viewport_array
	g_features.viewport_array =
		glViewportArrayv &&
		(gl_has_ext("GL_ARB_viewport_array") ||
			gl_has_ext("GL_EXT_viewport_array") ||
			(major > 4 || (major == 4 && minor >= 1)));
#endif

	// -------------------------------------------------------------------------
	// Texture compression (extension only, no API calls)
	// -------------------------------------------------------------------------
#if GL_ARB_texture_compression_bptc
	g_features.tex_compression_bptc =
		gl_has_ext("GL_ARB_texture_compression_bptc") ||
		gl_has_ext("GL_EXT_texture_compression_bptc");
#endif

#if GL_EXT_texture_compression_s3tc
	g_features.tex_compression_s3tc =
		gl_has_ext("GL_EXT_texture_compression_s3tc") ||
		gl_has_ext("GL_NV_texture_compression_s3tc");
#endif

#if GL_KHR_texture_compression_astc_ldr || GL_KHR_texture_compression_astc_hdr
	g_features.tex_compression_astc =
		gl_has_ext("GL_KHR_texture_compression_astc_ldr") ||
		gl_has_ext("GL_KHR_texture_compression_astc_hdr");
#endif

#if GL_ARB_ES3_compatibility || GL_OES_compressed_ETC2_RGB8_texture
	g_features.tex_compression_etc2 =
		gl_has_ext("GL_ARB_ES3_compatibility") ||
		gl_has_ext("GL_OES_compressed_ETC2_RGB8_texture");
#endif

	// -------------------------------------------------------------------------
	// Vendor-specific fixes
	// -------------------------------------------------------------------------
	const char* vendor = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);

	if (vendor && renderer)
	{
		bool is_broadcom_v3d =
			strstr(vendor, "Broadcom") &&
			strstr(renderer, "V3D");

		if (is_broadcom_v3d)
		{
			// Broadcom has buggy SSO
			g_features.sso = false;
		}
	}

	// Print final feature list
	bx::logf(bx::log_t::DEBUG, "OpenGL Feature Support:");

	bx::logf(bx::log_t::DEBUG, "\tSSO (Separable Shader Objects) : {}", g_features.sso ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tDSA (Direct State Access)      : {}", g_features.dsa ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tBuffer Storage                 : {}", g_features.buffer_storage ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tPersistent Mapping             : {}", g_features.persistent_mapping ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tSSBO                           : {}", g_features.ssbo ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tAtomic Counters                : {}", g_features.atomic_counters ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tShader Image Load/Store        : {}", g_features.shader_image_load_store ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tCompute Shader                 : {}", g_features.compute_shader ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tMulti Draw					 : {}", g_features.multi_draw ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tDraw Indirect                  : {}", g_features.draw_indirect ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tMulti Draw Indirect            : {}", g_features.multi_draw_indirect ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tIndirect Count                 : {}", g_features.indirect_count ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tTexture Storage                : {}", g_features.texture_storage ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tTexture View                   : {}", g_features.texture_view ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tBindless Textures              : {}", g_features.bindless_textures ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tSparse Texture                 : {}", g_features.sparse_texture ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tVertex Attrib Binding          : {}", g_features.vertex_attrib_binding ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tVAO                            : {}", g_features.vao ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tMulti Bind                     : {}", g_features.multi_bind ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tTessellation Shader            : {}", g_features.tessellation_shader ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tGeometry Shader                : {}", g_features.geometry_shader ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tShader Texture LOD             : {}", g_features.shader_texture_lod ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tAdvanced Blend                 : {}", g_features.advanced_blend ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tFramebuffer No Attachments     : {}", g_features.framebuffer_no_attachments ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tDebug Output                   : {}", g_features.debug_output ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tTimer Query                    : {}", g_features.timer_query ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tClip Control                   : {}", g_features.clip_control ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tViewport Array                 : {}", g_features.viewport_array ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tTexture Compression BPTC       : {}", g_features.tex_compression_bptc ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tTexture Compression S3TC       : {}", g_features.tex_compression_s3tc ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tTexture Compression ASTC       : {}", g_features.tex_compression_astc ? "YES" : "NO");
	bx::logf(bx::log_t::DEBUG, "\tTexture Compression ETC2       : {}", g_features.tex_compression_etc2 ? "YES" : "NO");
}

#if GL_ARB_debug_output || GL_KHR_debug
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
#endif

static void gl_setup_debug_callback()
{
#if GL_ARB_debug_output || GL_KHR_debug
	if (g_features.debug_output)
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
#endif
}

bool bx::gfx_init(const bx::app_config_t& config) bx_noexcept
{
	bx_profile(bx);

	gl_print_info();
	gl_check_features();
	gl_setup_debug_callback();

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

bx::handle_id bx::gfx_create_shader(const gfx_shader_desc_t& desc) bx_noexcept
{
	bx_profile(bx);

	const GLenum stage = gl_get_stage(desc.stage);
	if (stage == 0)
		return bx::invalid_handle;

	std::string source_code = GLSL_VERSION;
	if (!desc.source && desc.filepath)
	{
		std::ifstream file(desc.filepath);
		if (!file.is_open())
		{
			bx_loge(bx, "Failed to open file {}", desc.filepath);
			return bx::invalid_handle;
		}
		source_code += std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	}
	else
	{
		source_code += desc.source;
	}

	const char* src_ptr = source_code.c_str();
	GLuint program = 0;

	if (g_features.sso)
	{
		program = glCreateShaderProgramv(stage, 1, &src_ptr);
		if (!program)
		{
			bx_loge(bx, "gfx_create_shader: Failed to create shader program");
			return bx::invalid_handle;
		}
		glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);

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
	}
	else
	{
		const GLuint shader = glCreateShader(stage);
		if (!shader)
		{
			bx_logd(bx, "gfx_create_shader: Failed to create GL shader object.");
			return bx::invalid_handle;
		}

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
		program = shader;
	}

	gl_shader_t glshader{};
	glshader.shader = program;
	glshader.stage = stage;

	return g_shaders.insert(glshader);
}

void bx::gfx_destroy_shader(const handle_id handle) bx_noexcept
{
	bx_profile(bx);

	auto glsh = g_shaders.get(handle);
	if (!glsh) return;

	if (g_features.sso)
		glDeleteProgram(glsh->shader);
	else
		glDeleteShader(glsh->shader);
	
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

bx::handle_id bx::gfx_create_pipeline(const gfx_pipeline_desc_t& desc) bx_noexcept
{
	bx_profile(bx);

	if (!desc.shaders)
	{
		bx_loge(bx, "gfx_create_pipeline: no shaders provided");
		return bx::invalid_handle;
	}

	GLuint pipeline = 0;

	if (g_features.sso)
	{
		if (glCreateProgramPipelines)
			glCreateProgramPipelines(1, &pipeline);
		else
			glGenProgramPipelines(1, &pipeline);

		for (const auto sh : desc.shaders)
		{
			const auto glsh = g_shaders.get(sh);
			if (!glsh) continue;
			bx_ensure(glsh->shader != 0);

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
				if (glDeleteProgramPipelines)
					glDeleteProgramPipelines(1, &pipeline);
				else if (glDeleteProgramPipelinesEXT)
					glDeleteProgramPipelinesEXT(1, &pipeline);
				return invalid_handle;
			}

			glUseProgramStages(pipeline, stagebit, glsh->shader);
		}

		GLint status = 0;
		glValidateProgramPipeline(pipeline);
		glGetProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, &status);
		if (!status)
		{
			GLint logLen = 0;
			glGetProgramPipelineiv(pipeline, GL_INFO_LOG_LENGTH, &logLen);
			std::string log(logLen, '\0');
			glGetProgramPipelineInfoLog(pipeline, logLen, &logLen, &log[0]);
			bx_loge(bx, "Pipeline validation failed: {}", log);
		}

		//gl_set_debug_name(GL_PROGRAM, pipeline, -1, desc.name);
	}
	else
	{
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

		gl_set_debug_name(GL_PROGRAM, program, -1, desc.name);
		pipeline = program;
	}

	GLuint vao = 0;

	if (g_features.dsa)
	{
		glCreateVertexArrays(1, &vao);
	}
	else
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
	}

	if (glVertexArrayAttribFormat || glVertexAttribFormat)
	{
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

			const GLuint attrib_index = attr.location;
			const GLuint binding_index = attr.binding;
			const GLint count = attr.count;
			GLenum type = GL_FLOAT;
			GLint sizebytes = 0;
			gl_vattrib_info(attr.format, type, sizebytes);

			const GLboolean normalized = attr.normalized;
			const GLuint offset = attr.offset != 0 ? attr.offset : relative_offset;
		
			if (glCreateVertexArrays)
			{
				glEnableVertexArrayAttrib(vao, attrib_index);
				glVertexArrayAttribFormat(vao, attrib_index, count, type, normalized, offset);
				glVertexArrayAttribBinding(vao, attrib_index, binding_index);
				if (attr.input_rate_per_vertex_or_instance != 0)
					glVertexArrayBindingDivisor(vao, binding_index, attr.input_rate_per_vertex_or_instance);
			}
			else
			{
				glEnableVertexAttribArray(attrib_index);
				glVertexAttribFormat(attrib_index, count, type, normalized, offset);
				glVertexAttribBinding(attrib_index, binding_index);
				if (attr.input_rate_per_vertex_or_instance != 0)
					glVertexAttribDivisor(attrib_index, attr.input_rate_per_vertex_or_instance);
			}	

			relative_offset += sizebytes * count;
		}
	}

	if (!glCreateVertexArrays)
		glBindVertexArray(0);

	gl_set_debug_name(GL_VERTEX_ARRAY, vao, -1, desc.name);

	gl_pipeline_t glpipeline{};
	glpipeline.pipeline = pipeline;
	glpipeline.vao = vao;
	glpipeline.topology = desc.topology;
	glpipeline.raster = desc.raster;

	if (desc.input_layout.attributes)
	{
		glpipeline.attributes.assign(
			desc.input_layout.attributes.data,
			desc.input_layout.attributes.data + desc.input_layout.attributes.size);
	}

	if (desc.color_attachments)
	{
		glpipeline.blend_attachments.assign(
			desc.color_attachments.data,
			desc.color_attachments.data + desc.color_attachments.size);
	}

	return g_pipelines.insert(glpipeline);
}

void bx::gfx_destroy_pipeline(const handle_id handle) bx_noexcept
{
	bx_profile(bx);

	auto glpipeline = g_pipelines.get(handle);
	if (!glpipeline) return;

	if (g_features.sso)
	{
		if (glDeleteProgramPipelines)
			glDeleteProgramPipelines(1, &glpipeline->pipeline);
		else if (glDeleteProgramPipelinesEXT)
			glDeleteProgramPipelinesEXT(1, &glpipeline->pipeline);
	}
	else
	{
		glDeleteProgram(glpipeline->pipeline);
	}

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
	if (!glpipeline)
	{
		bx_logw(bx, "Attempted to bind pipeline with non-existent handle.");
		return;
	}

	g_current_pipeline = pipeline;

	if (g_features.sso)
		glBindProgramPipeline(glpipeline->pipeline);
	else
		glUseProgram(glpipeline->pipeline);

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

	auto glpipeline = g_pipelines.get(g_current_pipeline);
	if (!glpipeline)
	{
		bx_loge(bx, "Attemping to bind vertex buffers without a bound pipeline!");
		return;
	}

	if (glBindVertexBuffers || glBindVertexBuffer)
	{
		if (glBindVertexBuffers)
		{
			static GLuint tmp_buffers[MAX_BOUND_VERTEX_BUFFERS]{};
			static GLintptr tmp_offset[MAX_BOUND_VERTEX_BUFFERS]{};
			static GLsizei tmp_strides[MAX_BOUND_VERTEX_BUFFERS]{};
			for (i32 i = 0; i < binding_count; i++)
			{
				const auto glbuff = g_buffers.get(vertex_buffers[i]);
				if (!glbuff) continue;

				GLsizei stride = 0;
				for (const auto& attr : glpipeline->attributes)
				{
					if (attr.binding != i) continue;

					GLenum type = GL_FLOAT;
					GLint sizebytes = 0;
					gl_vattrib_info(attr.format, type, sizebytes);
					stride += sizebytes * attr.count;
				}

				tmp_buffers[i] = glbuff->id;
				tmp_offset[i] = 0;
				tmp_strides[i] = stride;
			}
			glBindVertexBuffers(first_binding, binding_count, tmp_buffers, tmp_offset, tmp_strides);
		}
		else
		{
			for (u32 i = 0; i < binding_count; ++i)
			{
				const auto glbuff = g_buffers.get(vertex_buffers[i]);
				if (!glbuff) continue;

				GLsizei stride = 0;
				for (const auto& attr : glpipeline->attributes)
				{
					if (attr.binding != i) continue;

					GLenum type = GL_FLOAT;
					GLint sizebytes = 0;
					gl_vattrib_info(attr.format, type, sizebytes);
					stride += sizebytes * attr.count;
				}

				const GLuint binding_index = first_binding + i;
				const GLintptr offset = static_cast<GLintptr>(offsets ? offsets[i] : 0);
				glBindVertexBuffer(binding_index, glbuff->id, offset, stride);
			}
		}
	}
	else
	{
		for (u32 i = 0; i < binding_count; ++i)
		{
			const auto glbuff = g_buffers.get(vertex_buffers[i]);
			if (!glbuff) continue;

			glBindBuffer(glbuff->target, glbuff->id);

			GLsizei stride = 0;
			for (const auto& attr : glpipeline->attributes)
			{
				if (attr.binding != i) continue;

				GLenum type = GL_FLOAT;
				GLint sizebytes = 0;
				gl_vattrib_info(attr.format, type, sizebytes);
				stride += sizebytes * attr.count;
			}

			GLuint relative_offset = 0;
			for (const auto& attr : glpipeline->attributes)
			{
				if (attr.binding != i) continue;

				const GLuint attrib_index = attr.location;
				const GLint count = attr.count;
				GLenum type = GL_FLOAT;
				GLint sizebytes = 0;
				gl_vattrib_info(attr.format, type, sizebytes);

				const GLboolean normalized = attr.normalized;
				const GLuint offset = attr.offset != 0 ? attr.offset : relative_offset;
				cvptr offset_ptr = reinterpret_cast<vptr>(offset);

				glEnableVertexAttribArray(attrib_index);
				glVertexAttribPointer(attrib_index, count, type, normalized, stride, offset_ptr);

				if (attr.input_rate_per_vertex_or_instance != 0)
					glVertexAttribDivisor(attrib_index, attr.input_rate_per_vertex_or_instance);

				relative_offset += sizebytes * count;
			}
		}
	}
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