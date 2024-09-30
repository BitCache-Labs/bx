#pragma once

#include "bx/engine/core/math.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/core/macros.hpp"
#include "bx/engine/containers/handle.hpp"
#include "bx/engine/containers/optional.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/hash_map.hpp"

struct BufferApi {};
using BufferHandle = Handle<BufferApi>;
struct SamplerApi {};
using SamplerHandle = Handle<SamplerApi>;
struct TextureApi {};
using TextureHandle = Handle<TextureApi>;
struct TextureViewApi {};
using TextureViewHandle = Handle<TextureViewApi>;
struct ShaderApi {};
using ShaderHandle = Handle<ShaderApi>;
struct GraphicsPipelineApi {};
using GraphicsPipelineHandle = Handle<GraphicsPipelineApi>;
struct ComputePipelineApi {};
using ComputePipelineHandle = Handle<ComputePipelineApi>;
struct RenderPassApi {};
using RenderPassHandle = Handle<RenderPassApi>;
struct ComputePassApi {};
using ComputePassHandle = Handle<ComputePassApi>;
struct BindGroupLayoutApi {};
using BindGroupLayoutHandle = Handle<BindGroupLayoutApi>;
struct BindGroupApi {};
using BindGroupHandle = Handle<BindGroupApi>;
struct BlasApi {};
using BlasHandle = Handle<BlasApi>;
struct TlasApi {};
using TlasHandle = Handle<TlasApi>;

ENUM(ShaderType,
	VERTEX,
	FRAGMENT,
	GEOMETRY,
	COMPUTE
);

ENUM(ShaderStageFlags,
	VERTEX = BX_BIT(0),
	FRAGMENT = BX_BIT(1),
	COMPUTE = BX_BIT(2)
);

ENUM(BufferUsageFlags,
	MAP_READ = BX_BIT(0),
	MAP_WRITE = BX_BIT(1),
	COPY_SRC = BX_BIT(2),
	COPY_DST = BX_BIT(3),
	INDEX = BX_BIT(4),
	VERTEX = BX_BIT(5),
	UNIFORM = BX_BIT(6),
	STORAGE = BX_BIT(7),
	INDIRECT = BX_BIT(8)
);

ENUM(StorageTextureAccess,
	READ,
	WRITE,
	READ_WRITE
);

ENUM(TextureFormat,
	R8_UNORM,
	R8_SNORM,
	R8_UINT,
	R8_SINT,

	R16_UINT,
	R16_SINT,
	R16_UNORM,
	R16_SNORM,
	R16_FLOAT,
	RG8_UNORM,
	RG8_SNORM,
	RG8_UINT,
	RG8_SINT,

	R32_UINT,
	R32_SINT,
	R32_FLOAT,
	RG16_UINT,
	RG16_SINT,
	RG16_UNORM,
	RG16_SNORM,
	RG16_FLOAT,
	RGBA8_UNORM,
	RGBA8_UNORM_SRGB,
	RGBA8_SNORM,
	RGBA8_UINT,
	RGBA8_SINT,
	BGRA8_UNORM,
	BGRA8_UNORM_SRGB,

	RGB9E5_UFLOAT,
	RGB10A2_UINT,
	RGB10A2_UNORM,
	RG11B10_FLOAT,

	RG32_UINT,
	RG32_SINT,
	RG32_FLOAT,
	RGBA16_UINT,
	RGBA16_SINT,
	RGBA16_UNORM,
	RGBA16_SNORM,
	RGBA16_FLOAT,

	RGBA32_UINT,
	RGBA32_SINT,
	RGBA32_FLOAT,

	STENCIL8,
	DEPTH16_UNORM,
	DEPTH24_PLUS,
	DEPTH24_PLUS_STENCIL8,
	DEPTH32_FLOAT,
	DEPTH32_FLOAT_STENCIL8,

	BC1_RGBA_UNORM,
	BC1_RGBA_UNORM_SRGB,
	BC2_RGBA_UNORM,
	BC2_RGBA_UNORM_SRGB,
	BC3_RGBA_UNORM,
	BC3_RGBA_UNORM_SRGB,
	BC4_R_UNORM,
	BC4_R_SNORM,
	BC5_RG_UNORM,
	BC5_RG_SNORM,
	BC6H_RGB_UFLOAT,
	BC6H_RGB_FLOAT,
	BC7_RGBA_UNORM,
	BC7_RGBA_UNORM_SRGB
);
ENUM(TextureUsageFlags,
	COPY_SRC = BX_BIT(0),
	COPY_DST = BX_BIT(1),
	TEXTURE_BINDING = BX_BIT(2),
	STORAGE_BINDING = BX_BIT(3),
	RENDER_ATTACHMENT = BX_BIT(4)
);

ENUM(SamplerBorderColor,
	TRANSPARENT_BLACK,
	OPAQUE_BLACK,
	OPAQUE_WHITE
);

ENUM(SamplerAddressMode,
	CLAMP_TO_EDGE,
	REPEAT,
	MIRROR_REPEAT,
	CLAMP_TO_BORDER
);

ENUM(FilterMode,
	NEAREST,
	LINEAR
);

ENUM(PrimitiveTopology,
	POINT_LIST,
	LINE_LIST,
	LINE_STRIP,
	TRIANGLE_LIST,
	TRIANGLE_STRIP,
);
ENUM(FrontFace,
	CCW,
	CW
);
ENUM(Face,
	FRONT,
	BACK
);

ENUM(VertexFormat,
	UINT_8X2,
	UINT_8X4,
	SINT_8X2,
	SINT_8X4,
	UNORM_8X2,
	UNORM_8X4,
	SNORM_8X2,
	SNORM_8X4,

	UINT_16X2,
	UINT_16X4,
	SINT_16X2,
	SINT_16X4,
	UNORM_16X2,
	UNORM_16X4,
	SNORM_16X2,
	SNORM_16X4,
	FLOAT_16X2,
	FLOAT_16X4,

	FLOAT_32,
	FLOAT_32X2,
	FLOAT_32X3,
	FLOAT_32X4,
	UINT_32,
	UINT_32X2,
	UINT_32X3,
	UINT_32X4,
	SINT_32,
	SINT_32X2,
	SINT_32X3,
	SINT_32X4,
);

ENUM(BlendFactor,
	ZERO,
	ONE,
	SRC,
	ONE_MINUS_SRC,
	SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA,
	DST,
	ONE_MINUS_DST,
	DST_ALPHA,
	ONE_MINUS_DST_ALPHA,
	SRC_ALPHA_SATURATED
);

ENUM(BlendOperation,
	ADD,
	SUBTRACT,
	REVERSE_SUBTRACT,
	MIN,
	MAX
);

ENUM(BindingType,
	UNIFORM_BUFFER,
	STORAGE_BUFFER,
	SAMPLER,
	TEXTURE,
	STORAGE_TEXTURE,
	ACCELERATION_STRUCTURE
);

ENUM(BindingResourceType,
	BUFFER,
	BUFFER_ARRAY,
	SAMPLER,
	SAMPLER_ARRAY,
	TEXTURE_VIEW,
	TEXTURE_VIEW_ARRAY,
	ACCELERATION_STRUCTURE
);

ENUM(TextureSampleType,
	FLOAT,
	DEPTH,
	UINT,
	SINT
);

ENUM(TextureViewDimension,
	D1,
	D2,
	D2_ARRAY,
	CUBE,
	CUBE_ARRAY,
	D3
);

ENUM(TextureDimension,
	D1,
	D2,
	D3
);

ENUM(LoadOp,
	CLEAR,
	LOAD
);

ENUM(StoreOp,
	STORE,
	DISCARD
);

ENUM(IndexFormat,
	UINT16,
	UINT32
);

struct Extend3D
{
	Extend3D() = default;
	Extend3D(u32 width, u32 height, u32 depthOrArrayLayers)
		: width(width), height(height), depthOrArrayLayers(depthOrArrayLayers) {}

	b8 operator==(const Extend3D& other) const
	{
		return width == other.width &&
			height == other.height &&
			depthOrArrayLayers == other.depthOrArrayLayers;
	}

	u32 width = 1;
	u32 height = 1;
	u32 depthOrArrayLayers = 1;
};

struct VertexAttribute
{
	VertexAttribute(VertexFormat format, u32 offset, u32 location)
		: format(format), offset(offset), location(location) {}

	VertexFormat format;
	u32 offset;
	u32 location;
};

struct VertexBufferLayout
{
	u32 stride;
	List<VertexAttribute> attributes;
};

struct BlendComponent
{
	static BlendComponent Replace();
	static BlendComponent AlphaBlending();

	BlendFactor srcFactor;
	BlendFactor dstFactor;
	BlendOperation operation = BlendOperation::ADD;
};

struct BlendState
{
	BlendComponent color;
	BlendComponent alpha;
};

struct ColorTargetState
{
	TextureFormat format = TextureFormat::RGBA8_UNORM_SRGB;
	Optional<BlendState> blend = Optional<BlendState>::None();
};

struct BindingTypeDescriptor
{
	static BindingTypeDescriptor UniformBuffer();
	static BindingTypeDescriptor StorageBuffer(b8 readOnly = true);
	static BindingTypeDescriptor Sampler();
	static BindingTypeDescriptor Texture(TextureSampleType sampleType, TextureViewDimension viewDimension = TextureViewDimension::D2, b8 multisampled = false);
	static BindingTypeDescriptor StorageTexture(StorageTextureAccess access, TextureFormat format, TextureViewDimension viewDimension = TextureViewDimension::D2);
	static BindingTypeDescriptor AccelerationStructure();

	BindingType type;

	union
	{
		struct
		{
			u32 _padding[4];
		} uniformBuffer;

		struct
		{
			b8 readOnly = true;
			u32 _padding[3];
		} storageBuffer;

		struct
		{
			u32 _padding[4];
		} sampler;

		struct
		{
			TextureSampleType sampleType = TextureSampleType::FLOAT;
			TextureViewDimension viewDimension = TextureViewDimension::D2;
			b8 multisampled = false;
			u32 _padding[1];
		} texture;

		struct
		{
			StorageTextureAccess access = StorageTextureAccess::READ;
			TextureFormat format = TextureFormat::RGBA8_UNORM_SRGB;
			TextureViewDimension viewDimension = TextureViewDimension::D2;
			u32 _padding[1];
		} storageTexture;

		struct
		{
			u32 _padding[4];
		} accelerationStructure;
	};
};

struct BindGroupLayoutEntry
{
	BindGroupLayoutEntry(u32 binding, ShaderStageFlags visibility, BindingTypeDescriptor type, const Optional<u32>& count = Optional<u32>::None())
		: binding(binding), visibility(visibility), type(type), count(count) {}

	u32 binding;
	ShaderStageFlags visibility;
	BindingTypeDescriptor type;
	Optional<u32> count = Optional<u32>::None();
};

struct BufferBinding
{
	BufferBinding() = default;
	BufferBinding(BufferHandle buffer, u64 offset = 0, const Optional<u64>& size = Optional<u64>::None())
		: buffer(buffer), offset(offset), size(size) {}

	BufferHandle buffer = BufferHandle::null;
	u64 offset = 0;
	Optional<u64> size = Optional<u64>::None();
};

struct BindingResource
{
	static BindingResource Buffer(const BufferBinding& bufferBinding);
	static BindingResource BufferArray(const List<BufferBinding>& bufferBindings);
	static BindingResource Sampler(const SamplerHandle& sampler);
	static BindingResource SamplerArray(const List<SamplerHandle>& samplers);
	static BindingResource TextureView(const TextureViewHandle& textureView);
	static BindingResource TextureViewArray(const List<TextureViewHandle>& textureViews);
	static BindingResource AccelerationStructure(const TlasHandle& accelerationStructure);

	BindingResourceType type;

	// TODO: can this be unioned somehow? std::vector seems to have trouble and the custom Optional<T> as well
	BufferBinding buffer;
	SamplerHandle sampler;
	TextureViewHandle textureView;
	TlasHandle accelerationStructure;
	List<BufferBinding> bufferArray = List<BufferBinding>{};
	List<SamplerHandle> samplerArray = List<SamplerHandle>{};
	List<TextureViewHandle> textureViewArray = List<TextureViewHandle>{};
};

struct BindGroupEntry
{
	BindGroupEntry(u32 binding, const BindingResource& resource)
		: binding(binding), resource(resource) {}

	u32 binding;
	BindingResource resource;
};

struct BindGroupCreateInfo
{
	String name = "Bind Group";

	BindGroupLayoutHandle layout = BindGroupLayoutHandle::null;
	List<BindGroupEntry> entries = List<BindGroupEntry>{};
};

struct BindGroupLayoutDescriptor
{
	BindGroupLayoutDescriptor(u32 group, const List<BindGroupLayoutEntry>& entries)
		: group(group), entries(entries) {}

	u32 group;
	List<BindGroupLayoutEntry> entries = List<BindGroupLayoutEntry>{};
};

struct PipelineLayoutDescriptor
{
	List<BindGroupLayoutDescriptor> bindGroupLayouts = List<BindGroupLayoutDescriptor>{};
};

struct GraphicsPipelineCreateInfo
{
	String name = "Graphics Pipeline";

	ShaderHandle vertexShader = ShaderHandle::null;
	ShaderHandle fragmentShader = ShaderHandle::null;
	// TODO
	// ShaderHandle geometryShader = ShaderHandle::null;
	// ShaderHandle tessalationShader = ShaderHandle::null;

	List<VertexBufferLayout> vertexBuffers = List<VertexBufferLayout>{};
	List<ColorTargetState> colorTargets = {};
	PrimitiveTopology topology = PrimitiveTopology::TRIANGLE_LIST;
	FrontFace frontFace = FrontFace::CCW;
	Optional<Face> cullMode = Optional<Face>::None();
	PipelineLayoutDescriptor layout;
	Optional<TextureFormat> depthFormat = Optional<TextureFormat>::None();
};

struct ComputePipelineCreateInfo
{
	String name = "Compute Pipeline";

	ShaderHandle shader = ShaderHandle::null;
	PipelineLayoutDescriptor layout;
	HashMap<String, f64> constants = HashMap<String, f64>{};
};

struct ShaderIncludeRange
{
	String name;
	u32 startLine;
	u32 endLine;
};

struct ShaderSrc
{
	ShaderSrc() = default;
	ShaderSrc(const String& src);

	String src;
	List<ShaderIncludeRange> includeRanges;
};

struct ShaderCreateInfo
{
	String name = "Shader";

	ShaderType shaderType;
	ShaderSrc src;
};

struct BufferCreateInfo
{
	String name = "Buffer";

	BufferUsageFlags usageFlags = 0;
	u64 size;
	const void* data = nullptr;

	BufferCreateInfo WithoutData() const
	{
		auto info = *this;
		info.data = nullptr;
		return info;
	}
};

struct SamplerCreateInfo
{
	String name = "Sampler";

	SamplerAddressMode addressModeU = SamplerAddressMode::CLAMP_TO_EDGE;
	SamplerAddressMode addressModeV = SamplerAddressMode::CLAMP_TO_EDGE;
	SamplerAddressMode addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
	FilterMode magFilter;
	FilterMode minFilter;
	f32 lodMinClamp = 0.0f;
	f32 lodMaxClamp = 32.0f;
	u16 anisotropyClamp = 1;
	Optional<SamplerBorderColor> borderColor = Optional<SamplerBorderColor>::None();
};

struct TextureCreateInfo
{
	String name = "Texture";

	Extend3D size = Extend3D{};
	u32 mipLevelCount = 1;
	u32 sampleCount = 1;
	TextureDimension dimension = TextureDimension::D2;
	TextureFormat format = TextureFormat::RGBA8_UNORM_SRGB;
	TextureUsageFlags usageFlags = 0;
	const void* data = nullptr;

	TextureCreateInfo WithoutData() const
	{
		auto info = *this;
		info.data = nullptr;
		return info;
	}
};

struct TextureViewCreateInfo
{
	TextureViewCreateInfo() = default;
	TextureViewCreateInfo(TextureHandle texture)
		: texture(texture) {}

	TextureHandle texture = TextureHandle::null;
	u32 baseMipLevel = 0;
	Optional<u32> mipLevelCount = Optional<u32>::None();
	u32 baseArrayLayer = 0;
	Optional<u32> arrayLayerCount = Optional<u32>::None();
};

struct ImageDataLayout
{
	u64 offset;
	// This value is required if there are multiple rows (i.e. height or depth is more than one pixel or pixel block for compressed textures)
	Optional<u32> bytesPerRow = Optional<u32>::None();
	// Required if there are multiple images (i.e. the depth is more than one).
	Optional<u32> rowsPerImage = Optional<u32>::None();
};

struct BufferSlice
{
	BufferSlice() = default;
	BufferSlice(BufferHandle buffer, u64 offset = 0, const Optional<u64>& size = Optional<u64>::None())
		: buffer(buffer), offset(offset), size(size) {}

	BufferHandle buffer = BufferHandle::null;
	u64 offset = 0;
	Optional<u64> size = Optional<u64>::None();
};

struct BlasCreateInfo
{
	String name = "Blas";

	BufferSlice vertexBuffer{};
	VertexFormat vertexFormat = VertexFormat::FLOAT_32X3;
	u32 vertexStride = 0;
	BufferSlice indexBuffer{};
	IndexFormat indexFormat = IndexFormat::UINT32;
};

struct BlasInstance
{
	Mat4 transform = Mat4::Identity();
	u32 instanceCustomIndex = 0;
	u8 mask = 0;
	BlasHandle blas = BlasHandle::null;
};

struct TlasCreateInfo
{
	String name = "Tlas";

	List<BlasInstance> blasInstances{};
};

struct Operations
{
	Operations(LoadOp load = LoadOp::LOAD, StoreOp store = StoreOp::STORE, const Optional<Color>& clearColor = Optional<Color>::None())
		: load(load), store(store), clearColor(clearColor) {}

	LoadOp load = LoadOp::LOAD;
	StoreOp store = StoreOp::STORE;

	Optional<Color> clearColor = Optional<Color>::None();
};

struct RenderPassColorAttachment
{
	RenderPassColorAttachment(TextureViewHandle view, Operations ops = Operations{}, const Optional<TextureViewHandle>& resolveTarget = Optional<TextureViewHandle>::None())
		: view(view), ops(ops), resolveTarget(resolveTarget) {}

	TextureViewHandle view = TextureViewHandle::null;
	Operations ops = Operations{};
	Optional<TextureViewHandle> resolveTarget = Optional<TextureViewHandle>::None();
};

struct RenderPassDepthStencilAttachment
{
	RenderPassDepthStencilAttachment(TextureViewHandle view, const Optional<Operations>& depthOps = Optional<Operations>::None(), const Optional<Operations>& stencilOps = Optional<Operations>::None())
		: view(view), depthOps(depthOps), stencilOps(stencilOps) {}

	TextureViewHandle view = TextureViewHandle::null;
	Optional<Operations> depthOps = Optional<Operations>::None();
	Optional<Operations> stencilOps = Optional<Operations>::None();
};

struct RenderPassDescriptor
{
	String name = "Render Pass";

	List<RenderPassColorAttachment> colorAttachments = List<RenderPassColorAttachment>{};
	Optional<RenderPassDepthStencilAttachment> depthStencilAttachment = Optional<RenderPassDepthStencilAttachment>::None();
};

struct ComputePassDescriptor
{
	String name = "Compute Pass";
};

struct GraphicsStats
{
	u32 bufferCount = 0;
	u32 samplerCount = 0;
	u32 textureCount = 0;
	u32 shaderCount = 0;
	u32 graphicsPipelineCount = 0;
	u32 computePipelineCount = 0;
	u32 bindGroupCount = 0;
	u32 blasCount = 0;
	u32 tlasCount = 0;
};

struct GraphicsCapabilities
{
	b8 raytracing = false;
};

b8 IsVertexFormatInt(const VertexFormat& format);
b8 IsTextureFormatSrgb(const TextureFormat& format);
b8 IsTextureFormatDepth(const TextureFormat& format);
b8 IsTextureFormatStencil(const TextureFormat& format);
u32 SizeOfTextureFormat(const TextureFormat& format);
u32 SizeOfIndexFormat(const IndexFormat& format);
b8 IsBufferUsageMappable(const BufferUsageFlags& usage);
u32 SizeOfTexturePixels(const TextureCreateInfo& info);

// TODO: remove
struct DebugVertex
{
	DebugVertex() {}
	DebugVertex(const Vec3& vert, u32 col)
		: vert(vert), col(col) {}

	Vec3 vert{ 0, 0, 0 };
	u32 col{ 0 };
};

struct DebugDrawAttribs
{
};