#include "bx/engine/modules/graphics/type.hpp"

#include "bx/engine/modules/graphics/type_validation.hpp"

ShaderSrc::ShaderSrc(const String& src)
	: src(src)
{

}

BlendComponent BlendComponent::Replace()
{
	BlendComponent component{};
	component.srcFactor = BlendFactor::ONE;
	component.dstFactor = BlendFactor::ZERO;
	return component;
}

BlendComponent BlendComponent::AlphaBlending()
{
	BlendComponent component{};
	component.srcFactor = BlendFactor::SRC_ALPHA;
	component.dstFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
	return component;
}

BindingTypeDescriptor BindingTypeDescriptor::UniformBuffer()
{
	BindingTypeDescriptor descriptor{};
	descriptor.type = BindingType::UNIFORM_BUFFER;
	return descriptor;
}

BindingTypeDescriptor BindingTypeDescriptor::StorageBuffer(b8 readOnly)
{
	BindingTypeDescriptor descriptor{};
	descriptor.type = BindingType::STORAGE_BUFFER;
	descriptor.storageBuffer.readOnly = readOnly;
	return descriptor;
}

BindingTypeDescriptor BindingTypeDescriptor::Sampler()
{
	BindingTypeDescriptor descriptor{};
	descriptor.type = BindingType::SAMPLER;
	return descriptor;
}

BindingTypeDescriptor BindingTypeDescriptor::Texture(TextureSampleType sampleType, TextureViewDimension viewDimension, b8 multisampled)
{
	BindingTypeDescriptor descriptor{};
	descriptor.type = BindingType::TEXTURE;
	descriptor.texture.sampleType = sampleType;
	descriptor.texture.viewDimension = viewDimension;
	descriptor.texture.multisampled = multisampled;
	return descriptor;
}

BindingTypeDescriptor BindingTypeDescriptor::StorageTexture(StorageTextureAccess access, TextureFormat format, TextureViewDimension viewDimension)
{
	BX_ASSERT(!IsTextureFormatSrgb(format), "Storage texture format cannot be srgb.");

	BindingTypeDescriptor descriptor{};
	descriptor.type = BindingType::STORAGE_TEXTURE;
	descriptor.storageTexture.access = access;
	descriptor.storageTexture.format = format;
	descriptor.storageTexture.viewDimension = viewDimension;
	return descriptor;
}

BindingTypeDescriptor BindingTypeDescriptor::AccelerationStructure()
{
	BindingTypeDescriptor descriptor{};
	descriptor.type = BindingType::ACCELERATION_STRUCTURE;
	return descriptor;
}

BindingResource BindingResource::Buffer(const BufferBinding& bufferBinding)
{
	BX_ENSURE(bufferBinding.buffer);

	BindingResource resource{};
	resource.type = BindingResourceType::BUFFER;
	resource.buffer = bufferBinding;
	return resource;
}

BindingResource BindingResource::BufferArray(const List<BufferBinding>& bufferBindings)
{
	BindingResource resource{};
	resource.type = BindingResourceType::BUFFER_ARRAY;
	resource.bufferArray = bufferBindings;
	return resource;
}

BindingResource BindingResource::Sampler(const SamplerHandle& sampler)
{
	BX_ENSURE(sampler);

	BindingResource resource{};
	resource.type = BindingResourceType::SAMPLER;
	resource.sampler = sampler;
	return resource;
}

BindingResource BindingResource::SamplerArray(const List<SamplerHandle>& samplers)
{
	BindingResource resource{};
	resource.type = BindingResourceType::SAMPLER_ARRAY;
	resource.samplerArray = samplers;
	return resource;
}

BindingResource BindingResource::TextureView(const TextureViewHandle& textureView)
{
	BX_ENSURE(textureView);

	BindingResource resource{};
	resource.type = BindingResourceType::TEXTURE_VIEW;
	resource.textureView = textureView;
	return resource;
}

BindingResource BindingResource::TextureViewArray(const List<TextureViewHandle>& textureViews)
{
	BindingResource resource{};
	resource.type = BindingResourceType::TEXTURE_VIEW_ARRAY;
	resource.textureViewArray = textureViews;
	return resource;
}

BindingResource BindingResource::AccelerationStructure(const TlasHandle& accelerationStructure)
{
	BX_ENSURE(accelerationStructure);

	BindingResource resource{};
	resource.type = BindingResourceType::ACCELERATION_STRUCTURE;
	resource.accelerationStructure = accelerationStructure;
	return resource;
}

b8 IsVertexFormatInt(const VertexFormat& format)
{
	switch (format)
	{
	case VertexFormat::UINT_8X2:
		return true;
	case VertexFormat::UINT_8X4:
		return true;
	case VertexFormat::SINT_8X2:
		return true;
	case VertexFormat::SINT_8X4:
		return true;
	case VertexFormat::UNORM_8X2:
		return false;
	case VertexFormat::UNORM_8X4:
		return false;
	case VertexFormat::SNORM_8X2:
		return false;
	case VertexFormat::SNORM_8X4:
		return false;

	case VertexFormat::UINT_16X2:
		return true;
	case VertexFormat::UINT_16X4:
		return true;
	case VertexFormat::SINT_16X2:
		return true;
	case VertexFormat::SINT_16X4:
		return true;
	case VertexFormat::UNORM_16X2:
		return false;
	case VertexFormat::UNORM_16X4:
		return false;
	case VertexFormat::SNORM_16X2:
		return false;
	case VertexFormat::SNORM_16X4:
		return false;
	case VertexFormat::FLOAT_16X2:
		return false;
	case VertexFormat::FLOAT_16X4:
		return false;

	case VertexFormat::FLOAT_32:
		return false;
	case VertexFormat::FLOAT_32X2:
		return false;
	case VertexFormat::FLOAT_32X3:
		return false;
	case VertexFormat::FLOAT_32X4:
		return false;
	case VertexFormat::UINT_32:
		return true;
	case VertexFormat::UINT_32X2:
		return true;
	case VertexFormat::UINT_32X3:
		return true;
	case VertexFormat::UINT_32X4:
		return true;
	case VertexFormat::SINT_32:
		return true;
	case VertexFormat::SINT_32X2:
		return true;
	case VertexFormat::SINT_32X3:
		return true;
	case VertexFormat::SINT_32X4:
		return true;
	default:
		BX_FAIL("Vertex format not supported.");
		return false;
	}
}

b8 IsTextureFormatSrgb(const TextureFormat& format)
{
	switch (format)
	{
	case TextureFormat::RGBA8_UNORM_SRGB:
		return true;
	case TextureFormat::BGRA8_UNORM_SRGB:
		return true;
	case TextureFormat::BC1_RGBA_UNORM_SRGB:
		return true;
	case TextureFormat::BC2_RGBA_UNORM_SRGB:
		return true;
	case TextureFormat::BC3_RGBA_UNORM_SRGB:
		return true;
	case TextureFormat::BC7_RGBA_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

b8 IsTextureFormatHdr(const TextureFormat& format)
{
	switch (format)
	{
	case TextureFormat::R16_FLOAT:
		return true;
	case TextureFormat::R32_FLOAT:
		return true;
	case TextureFormat::RG16_FLOAT:
		return true;
	case TextureFormat::RGB9E5_UFLOAT:
		return true;
	case TextureFormat::RG11B10_FLOAT:
		return true;
	case TextureFormat::RG32_FLOAT:
		return true;
	case TextureFormat::RGBA16_FLOAT:
		return true;
	case TextureFormat::RGBA32_FLOAT:
		return true;
	case TextureFormat::BC6H_RGB_UFLOAT:
		return true;
	case TextureFormat::BC6H_RGB_FLOAT:
		return true;
	default:
		return false;
	}
}

b8 IsTextureFormatDepth(const TextureFormat& format)
{
	switch (format)
	{
	case TextureFormat::STENCIL8:
		return false;
	case TextureFormat::DEPTH16_UNORM:
		return true;
	case TextureFormat::DEPTH24_PLUS:
		return true;
	case TextureFormat::DEPTH24_PLUS_STENCIL8:
		return true;
	case TextureFormat::DEPTH32_FLOAT:
		return true;
	case TextureFormat::DEPTH32_FLOAT_STENCIL8:
		return true;
	default:
		return false;
	}
}

b8 IsTextureFormatStencil(const TextureFormat& format)
{
	switch (format)
	{
	case TextureFormat::STENCIL8:
		return true;
	case TextureFormat::DEPTH16_UNORM:
		return false;
	case TextureFormat::DEPTH24_PLUS:
		return false;
	case TextureFormat::DEPTH24_PLUS_STENCIL8:
		return true;
	case TextureFormat::DEPTH32_FLOAT:
		return false;
	case TextureFormat::DEPTH32_FLOAT_STENCIL8:
		return true;
	default:
		return false;
	}
}

u32 SizeOfTextureFormat(const TextureFormat& format)
{
	switch (format)
	{
	case TextureFormat::R8_UNORM:
		return 1;
	case TextureFormat::R8_SNORM:
		return 1;
	case TextureFormat::R8_UINT:
		return 1;
	case TextureFormat::R8_SINT:
		return 1;

	case TextureFormat::R16_UINT:
		return 2;
	case TextureFormat::R16_SINT:
		return 2;
	case TextureFormat::R16_UNORM:
		return 2;
	case TextureFormat::R16_SNORM:
		return 2;
	case TextureFormat::R16_FLOAT:
		return 2;
	case TextureFormat::RG8_UNORM:
		return 2;
	case TextureFormat::RG8_SNORM:
		return 2;
	case TextureFormat::RG8_UINT:
		return 2;
	case TextureFormat::RG8_SINT:
		return 2;

	case TextureFormat::R32_UINT:
		return 4;
	case TextureFormat::R32_SINT:
		return 4;
	case TextureFormat::R32_FLOAT:
		return 4;
	case TextureFormat::RG16_UINT:
		return 4;
	case TextureFormat::RG16_SINT:
		return 4;
	case TextureFormat::RG16_UNORM:
		return 4;
	case TextureFormat::RG16_SNORM:
		return 4;
	case TextureFormat::RG16_FLOAT:
		return 4;
	case TextureFormat::RGBA8_UNORM:
		return 4;
	case TextureFormat::RGBA8_UNORM_SRGB:
		return 4;
	case TextureFormat::RGBA8_SNORM:
		return 4;
	case TextureFormat::RGBA8_UINT:
		return 4;
	case TextureFormat::RGBA8_SINT:
		return 4;
	case TextureFormat::BGRA8_UNORM:
		return 4;
	case TextureFormat::BGRA8_UNORM_SRGB:
		return 4;

	case TextureFormat::RGB9E5_UFLOAT:
		4;
	case TextureFormat::RGB10A2_UINT:
		return 4;
	case TextureFormat::RGB10A2_UNORM:
		return 4;
	case TextureFormat::RG11B10_FLOAT:
		return 4;

	case TextureFormat::RG32_UINT:
		return 8;
	case TextureFormat::RG32_SINT:
		return 8;
	case TextureFormat::RG32_FLOAT:
		return 8;
	case TextureFormat::RGBA16_UINT:
		return 8;
	case TextureFormat::RGBA16_SINT:
		return 8;
	case TextureFormat::RGBA16_UNORM:
		return 8;
	case TextureFormat::RGBA16_SNORM:
		return 8;
	case TextureFormat::RGBA16_FLOAT:
		return 8;

	case TextureFormat::RGBA32_UINT:
		return 16;
	case TextureFormat::RGBA32_SINT:
		return 16;
	case TextureFormat::RGBA32_FLOAT:
		return 16;

	case TextureFormat::STENCIL8:
		return 1;
	case TextureFormat::DEPTH16_UNORM:
		return 2;
	case TextureFormat::DEPTH24_PLUS:
		return 3;
	case TextureFormat::DEPTH24_PLUS_STENCIL8:
		return 4;
	case TextureFormat::DEPTH32_FLOAT:
		return 4;
	case TextureFormat::DEPTH32_FLOAT_STENCIL8:
		return 5;

		/*case TextureFormat::BC1_RGBA_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC1_RGBA_UNORM_SRGB:
			return GL_FLOAT;
		case TextureFormat::BC2_RGBA_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC2_RGBA_UNORM_SRGB:
			return GL_FLOAT;
		case TextureFormat::BC3_RGBA_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC3_RGBA_UNORM_SRGB:
			return GL_FLOAT;
		case TextureFormat::BC4_R_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC4_R_SNORM:
			return GL_FLOAT;
		case TextureFormat::BC5_RG_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC5_RG_SNORM:
			return GL_FLOAT;
		case TextureFormat::BC6H_RGB_UFLOAT:
			return GL_FLOAT;
		case TextureFormat::BC6H_RGB_FLOAT:
			return GL_FLOAT;
		case TextureFormat::BC7_RGBA_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC7_RGBA_UNORM_SRGB:
			return GL_FLOAT;*/

	default:
		BX_FAIL("Texture format not supported.");
		return 0;
	}
}

u32 ChannelsOfTextureFormat(const TextureFormat& format)
{
	switch (format)
	{
	case TextureFormat::R8_UNORM:
		return 1;
	case TextureFormat::R8_SNORM:
		return 1;
	case TextureFormat::R8_UINT:
		return 1;
	case TextureFormat::R8_SINT:
		return 1;

	case TextureFormat::R16_UINT:
		return 1;
	case TextureFormat::R16_SINT:
		return 1;
	case TextureFormat::R16_UNORM:
		return 1;
	case TextureFormat::R16_SNORM:
		return 1;
	case TextureFormat::R16_FLOAT:
		return 1;
	case TextureFormat::RG8_UNORM:
		return 2;
	case TextureFormat::RG8_SNORM:
		return 2;
	case TextureFormat::RG8_UINT:
		return 2;
	case TextureFormat::RG8_SINT:
		return 2;

	case TextureFormat::R32_UINT:
		return 1;
	case TextureFormat::R32_SINT:
		return 1;
	case TextureFormat::R32_FLOAT:
		return 1;
	case TextureFormat::RG16_UINT:
		return 2;
	case TextureFormat::RG16_SINT:
		return 2;
	case TextureFormat::RG16_UNORM:
		return 2;
	case TextureFormat::RG16_SNORM:
		return 2;
	case TextureFormat::RG16_FLOAT:
		return 2;
	case TextureFormat::RGBA8_UNORM:
		return 4;
	case TextureFormat::RGBA8_UNORM_SRGB:
		return 4;
	case TextureFormat::RGBA8_SNORM:
		return 4;
	case TextureFormat::RGBA8_UINT:
		return 4;
	case TextureFormat::RGBA8_SINT:
		return 4;
	case TextureFormat::BGRA8_UNORM:
		return 4;
	case TextureFormat::BGRA8_UNORM_SRGB:
		return 4;

	case TextureFormat::RGB9E5_UFLOAT:
		return 4;
	case TextureFormat::RGB10A2_UINT:
		return 4;
	case TextureFormat::RGB10A2_UNORM:
		return 4;
	case TextureFormat::RG11B10_FLOAT:
		return 3;

	case TextureFormat::RG32_UINT:
		return 2;
	case TextureFormat::RG32_SINT:
		return 2;
	case TextureFormat::RG32_FLOAT:
		return 2;
	case TextureFormat::RGBA16_UINT:
		return 4;
	case TextureFormat::RGBA16_SINT:
		return 4;
	case TextureFormat::RGBA16_UNORM:
		return 4;
	case TextureFormat::RGBA16_SNORM:
		return 4;
	case TextureFormat::RGBA16_FLOAT:
		return 4;

	case TextureFormat::RGBA32_UINT:
		return 4;
	case TextureFormat::RGBA32_SINT:
		return 4;
	case TextureFormat::RGBA32_FLOAT:
		return 4;

	case TextureFormat::STENCIL8:
		return 1;
	case TextureFormat::DEPTH16_UNORM:
		return 1;
	case TextureFormat::DEPTH24_PLUS:
		return 1;
	case TextureFormat::DEPTH24_PLUS_STENCIL8:
		return 2;
	case TextureFormat::DEPTH32_FLOAT:
		return 1;
	case TextureFormat::DEPTH32_FLOAT_STENCIL8:
		return 2;

		/*case TextureFormat::BC1_RGBA_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC1_RGBA_UNORM_SRGB:
			return GL_FLOAT;
		case TextureFormat::BC2_RGBA_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC2_RGBA_UNORM_SRGB:
			return GL_FLOAT;
		case TextureFormat::BC3_RGBA_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC3_RGBA_UNORM_SRGB:
			return GL_FLOAT;
		case TextureFormat::BC4_R_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC4_R_SNORM:
			return GL_FLOAT;
		case TextureFormat::BC5_RG_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC5_RG_SNORM:
			return GL_FLOAT;
		case TextureFormat::BC6H_RGB_UFLOAT:
			return GL_FLOAT;
		case TextureFormat::BC6H_RGB_FLOAT:
			return GL_FLOAT;
		case TextureFormat::BC7_RGBA_UNORM:
			return GL_FLOAT;
		case TextureFormat::BC7_RGBA_UNORM_SRGB:
			return GL_FLOAT;*/

	default:
		BX_FAIL("Texture format not supported.");
		return 0;
	}
}

u32 SizeOfIndexFormat(const IndexFormat& format)
{
	return format == IndexFormat::UINT32 ? 4 : 2;
}

b8 IsBufferUsageMappable(const BufferUsageFlags& usage)
{
	return usage & BufferUsageFlags::MAP_READ || usage & BufferUsageFlags::MAP_WRITE;
}

u32 SizeOfTexturePixels(const TextureCreateInfo& info)
{
	u32 pixels = info.size.width * info.size.height * info.size.depthOrArrayLayers;
	return pixels * SizeOfTextureFormat(info.format);
}