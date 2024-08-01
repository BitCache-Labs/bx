#include "bx/engine/modules/graphics/backend/opengl/conversion.hpp"

#include "bx/engine/core/macros.hpp"

namespace Gl
{
	GLenum TextureDimensionToGl(const TextureDimension& dimension, u32 depthOrArrayLayers)
	{
		switch (dimension)
		{
		case TextureDimension::D1:
			return (depthOrArrayLayers == 1) ? GL_TEXTURE_1D : GL_TEXTURE_1D_ARRAY;
		case TextureDimension::D2:
			return (depthOrArrayLayers == 1) ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
		case TextureDimension::D3:
			return GL_TEXTURE_3D;
		default:
			BX_FAIL("Texture dimension not supported.");
			return GL_INVALID_ENUM;
		}
	}

	GLenum TextureFormatToGlType(const TextureFormat& format)
	{
		switch (format)
		{
		case TextureFormat::R8_UNORM:
			return GL_UNSIGNED_BYTE;
		case TextureFormat::R8_SNORM:
			return GL_BYTE;
		case TextureFormat::R8_UINT:
			return GL_UNSIGNED_BYTE;
		case TextureFormat::R8_SINT:
			return GL_BYTE;

		case TextureFormat::R16_UINT:
			return GL_UNSIGNED_SHORT;
		case TextureFormat::R16_SINT:
			return GL_SHORT;
		case TextureFormat::R16_UNORM:
			return GL_UNSIGNED_SHORT;
		case TextureFormat::R16_SNORM:
			return GL_SHORT;
		case TextureFormat::R16_FLOAT:
			return GL_HALF_FLOAT;
		case TextureFormat::RG8_UNORM:
			return GL_UNSIGNED_BYTE;
		case TextureFormat::RG8_SNORM:
			return GL_BYTE;
		case TextureFormat::RG8_UINT:
			return GL_UNSIGNED_BYTE;
		case TextureFormat::RG8_SINT:
			return GL_BYTE;

		case TextureFormat::R32_UINT:
			return GL_UNSIGNED_INT;
		case TextureFormat::R32_SINT:
			return GL_INT;
		case TextureFormat::R32_FLOAT:
			return GL_FLOAT;
		case TextureFormat::RG16_UINT:
			return GL_UNSIGNED_SHORT;
		case TextureFormat::RG16_SINT:
			return GL_SHORT;
		case TextureFormat::RG16_UNORM:
			return GL_UNSIGNED_SHORT;
		case TextureFormat::RG16_SNORM:
			return GL_SHORT;
		case TextureFormat::RG16_FLOAT:
			return GL_HALF_FLOAT;
		case TextureFormat::RGBA8_UNORM:
			return GL_UNSIGNED_BYTE;
		case TextureFormat::RGBA8_UNORM_SRGB:
			return GL_UNSIGNED_BYTE;
		case TextureFormat::RGBA8_SNORM:
			return GL_BYTE;
		case TextureFormat::RGBA8_UINT:
			return GL_UNSIGNED_BYTE;
		case TextureFormat::RGBA8_SINT:
			return GL_BYTE;
		case TextureFormat::BGRA8_UNORM:
			return GL_UNSIGNED_BYTE;
		case TextureFormat::BGRA8_UNORM_SRGB:
			return GL_UNSIGNED_BYTE;

		case TextureFormat::RGB9E5_UFLOAT:
			GL_UNSIGNED_INT_5_9_9_9_REV;
		case TextureFormat::RGB10A2_UINT:
			return GL_UNSIGNED_INT_10_10_10_2;
		case TextureFormat::RGB10A2_UNORM:
			return GL_UNSIGNED_INT_10_10_10_2;
		case TextureFormat::RG11B10_FLOAT:
			return GL_FLOAT;

		case TextureFormat::RG32_UINT:
			return GL_UNSIGNED_INT;
		case TextureFormat::RG32_SINT:
			return GL_INT;
		case TextureFormat::RG32_FLOAT:
			return GL_FLOAT;
		case TextureFormat::RGBA16_UINT:
			return GL_UNSIGNED_SHORT;
		case TextureFormat::RGBA16_SINT:
			return GL_SHORT;
		case TextureFormat::RGBA16_UNORM:
			return GL_UNSIGNED_SHORT;
		case TextureFormat::RGBA16_SNORM:
			return GL_SHORT;
		case TextureFormat::RGBA16_FLOAT:
			return GL_HALF_FLOAT;

		case TextureFormat::RGBA32_UINT:
			return GL_UNSIGNED_INT;
		case TextureFormat::RGBA32_SINT:
			return GL_INT;
		case TextureFormat::RGBA32_FLOAT:
			return GL_FLOAT;

		// TODO: figure out how to support these
		case TextureFormat::STENCIL8:
			return GL_FLOAT;
		case TextureFormat::DEPTH16_UNORM:
			return GL_FLOAT;
		case TextureFormat::DEPTH24_PLUS:
			return GL_FLOAT;
		case TextureFormat::DEPTH24_PLUS_STENCIL8:
			return GL_UNSIGNED_INT_24_8_EXT;
		case TextureFormat::DEPTH32_FLOAT:
			return GL_FLOAT;
		case TextureFormat::DEPTH32_FLOAT_STENCIL8:
			return GL_FLOAT;

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
			return GL_INVALID_ENUM;
		}
	}

	GLenum TextureFormatToGlFormat(const TextureFormat& format)
	{
		switch (format)
		{
		case TextureFormat::R8_UNORM:
			return GL_RED;
		case TextureFormat::R8_SNORM:
			return GL_RED;
		case TextureFormat::R8_UINT:
			return GL_RED_INTEGER;
		case TextureFormat::R8_SINT:
			return GL_RED_INTEGER;

		case TextureFormat::R16_UINT:
			return GL_RED_INTEGER;
		case TextureFormat::R16_SINT:
			return GL_RED_INTEGER;
		case TextureFormat::R16_UNORM:
			return GL_RED;
		case TextureFormat::R16_SNORM:
			return GL_RED;
		case TextureFormat::R16_FLOAT:
			return GL_RED;
		case TextureFormat::RG8_UNORM:
			return GL_RG;
		case TextureFormat::RG8_SNORM:
			return GL_RG;
		case TextureFormat::RG8_UINT:
			return GL_RG_INTEGER;
		case TextureFormat::RG8_SINT:
			return GL_RG_INTEGER;

		case TextureFormat::R32_UINT:
			return GL_RED_INTEGER;
		case TextureFormat::R32_SINT:
			return GL_RED_INTEGER;
		case TextureFormat::R32_FLOAT:
			return GL_RED;
		case TextureFormat::RG16_UINT:
			return GL_RG_INTEGER;
		case TextureFormat::RG16_SINT:
			return GL_RG_INTEGER;
		case TextureFormat::RG16_UNORM:
			return GL_RG;
		case TextureFormat::RG16_SNORM:
			return GL_RG;
		case TextureFormat::RG16_FLOAT:
			return GL_RG;
		case TextureFormat::RGBA8_UNORM:
			return GL_RGBA;
		case TextureFormat::RGBA8_UNORM_SRGB:
			return GL_RGBA;
		case TextureFormat::RGBA8_SNORM:
			return GL_RGBA;
		case TextureFormat::RGBA8_UINT:
			return GL_RGBA_INTEGER;
		case TextureFormat::RGBA8_SINT:
			return GL_RGBA_INTEGER;
		/*case TextureFormat::BGRA8_UNORM:
			return GL_BGRA;*/
		/*case TextureFormat::BGRA8_UNORM_SRGB:
			return GL_UNSIGNED_BYTE;*/

		case TextureFormat::RGB9E5_UFLOAT:
			return GL_RGB;
		case TextureFormat::RGB10A2_UINT:
			return GL_RGBA_INTEGER;
		case TextureFormat::RGB10A2_UNORM:
			return GL_RGBA;
		case TextureFormat::RG11B10_FLOAT:
			return GL_RGB;

		case TextureFormat::RG32_UINT:
			return GL_RG_INTEGER;
		case TextureFormat::RG32_SINT:
			return GL_RG_INTEGER;
		case TextureFormat::RG32_FLOAT:
			return GL_RG;
		case TextureFormat::RGBA16_UINT:
			return GL_RGBA_INTEGER;
		case TextureFormat::RGBA16_SINT:
			return GL_RGBA_INTEGER;
		case TextureFormat::RGBA16_UNORM:
			return GL_RGBA;
		case TextureFormat::RGBA16_SNORM:
			return GL_RGBA;
		case TextureFormat::RGBA16_FLOAT:
			return GL_RGBA;

		case TextureFormat::RGBA32_UINT:
			return GL_RGBA_INTEGER;
		case TextureFormat::RGBA32_SINT:
			return GL_RGBA_INTEGER;
		case TextureFormat::RGBA32_FLOAT:
			return GL_RGBA;

			// TODO: figure out how to support these
			/*case TextureFormat::STENCIL8:
				return GL_BYTE;*/
		case TextureFormat::DEPTH16_UNORM:
			return GL_DEPTH_COMPONENT;
		case TextureFormat::DEPTH24_PLUS:
			return GL_DEPTH_COMPONENT;
		case TextureFormat::DEPTH24_PLUS_STENCIL8:
			return GL_DEPTH_STENCIL;
		case TextureFormat::DEPTH32_FLOAT:
			return GL_DEPTH_COMPONENT;
		case TextureFormat::DEPTH32_FLOAT_STENCIL8:
			return GL_DEPTH_STENCIL;

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
			return GL_INVALID_ENUM;
		}
	}

	GLenum TextureFormatToGlInternalFormat(const TextureFormat& format)
	{
		switch (format)
		{
		case TextureFormat::R8_UNORM:
			return GL_R8;
		case TextureFormat::R8_SNORM:
			return GL_R8_SNORM;
		case TextureFormat::R8_UINT:
			return GL_R8UI;
		case TextureFormat::R8_SINT:
			return GL_R8I;

		case TextureFormat::R16_UINT:
			return GL_R16UI;
		case TextureFormat::R16_SINT:
			return GL_R16I;
		case TextureFormat::R16_UNORM:
			return GL_R16;
		case TextureFormat::R16_SNORM:
			return GL_R16_SNORM;
		case TextureFormat::R16_FLOAT:
			return GL_R16F;
		case TextureFormat::RG8_UNORM:
			return GL_RG8;
		case TextureFormat::RG8_SNORM:
			return GL_RG8_SNORM;
		case TextureFormat::RG8_UINT:
			return GL_RG8UI;
		case TextureFormat::RG8_SINT:
			return GL_RG8I;

		case TextureFormat::R32_UINT:
			return GL_R32UI;
		case TextureFormat::R32_SINT:
			return GL_R32I;
		case TextureFormat::R32_FLOAT:
			return GL_R32F;
		case TextureFormat::RG16_UINT:
			return GL_RG16UI;
		case TextureFormat::RG16_SINT:
			return GL_RG16I;
		case TextureFormat::RG16_UNORM:
			return GL_RG16;
		case TextureFormat::RG16_SNORM:
			return GL_RG16_SNORM;
		case TextureFormat::RG16_FLOAT:
			return GL_RG16F;
		case TextureFormat::RGBA8_UNORM:
			return GL_RGBA8;
		case TextureFormat::RGBA8_UNORM_SRGB:
			return GL_SRGB8_ALPHA8;
		case TextureFormat::RGBA8_SNORM:
			return GL_RGBA8_SNORM;
		case TextureFormat::RGBA8_UINT:
			return GL_RGBA8UI;
		case TextureFormat::RGBA8_SINT:
			return GL_RGBA8I;
		/*case TextureFormat::BGRA8_UNORM:
			return GL_BGRA;*/
		/*case TextureFormat::BGRA8_UNORM_SRGB:
			return GL_UNSIGNED_BYTE;*/

		case TextureFormat::RGB9E5_UFLOAT:
			GL_RGB9_E5;
		case TextureFormat::RGB10A2_UINT:
			return GL_RGB10_A2UI;
		case TextureFormat::RGB10A2_UNORM:
			return GL_RGB10_A2;
		case TextureFormat::RG11B10_FLOAT:
			return GL_R11F_G11F_B10F;

		case TextureFormat::RG32_UINT:
			return GL_RG32UI;
		case TextureFormat::RG32_SINT:
			return GL_RG32I;
		case TextureFormat::RG32_FLOAT:
			return GL_RG32F;
		case TextureFormat::RGBA16_UINT:
			return GL_RGBA16UI;
		case TextureFormat::RGBA16_SINT:
			return GL_RGBA16I;
		case TextureFormat::RGBA16_UNORM:
			return GL_RGBA16;
		case TextureFormat::RGBA16_SNORM:
			return GL_RGBA16_SNORM;
		case TextureFormat::RGBA16_FLOAT:
			return GL_RGBA16F;

		case TextureFormat::RGBA32_UINT:
			return GL_RGBA32UI;
		case TextureFormat::RGBA32_SINT:
			return GL_RGBA32I;
		case TextureFormat::RGBA32_FLOAT:
			return GL_RGBA32F;

		// TODO: figure out how to support these
		/*case TextureFormat::STENCIL8:
			return GL_BYTE;*/
		case TextureFormat::DEPTH16_UNORM:
			return GL_DEPTH_COMPONENT;
		case TextureFormat::DEPTH24_PLUS:
			return GL_DEPTH_COMPONENT;
		case TextureFormat::DEPTH24_PLUS_STENCIL8:
			return GL_DEPTH_STENCIL;
		case TextureFormat::DEPTH32_FLOAT:
			return GL_DEPTH_COMPONENT;
		case TextureFormat::DEPTH32_FLOAT_STENCIL8:
			return GL_DEPTH_STENCIL;

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
			return GL_INVALID_ENUM;
		}
	}

	GLenum ShaderTypeToGl(const ShaderType& type)
	{
		switch (type)
		{
		case ShaderType::VERTEX:
			return GL_VERTEX_SHADER;
		case ShaderType::FRAGMENT:
			return GL_FRAGMENT_SHADER;
		case ShaderType::GEOMETRY:
			return GL_GEOMETRY_SHADER;
		case ShaderType::COMPUTE:
			return GL_COMPUTE_SHADER;
		default:
			BX_FAIL("Shader type not supported.");
			return GL_INVALID_ENUM;
		}
	}

	GLenum BlendFactorToGl(const BlendFactor& factor)
	{
		switch (factor)
		{
		case BlendFactor::ZERO:
			return GL_ZERO;
		case BlendFactor::ONE:
			return GL_ONE;
		case BlendFactor::SRC:
			return GL_SRC_COLOR;
		case BlendFactor::ONE_MINUS_SRC:
			return GL_ONE_MINUS_SRC_COLOR;
		case BlendFactor::SRC_ALPHA:
			return GL_SRC_ALPHA;
		case BlendFactor::ONE_MINUS_SRC_ALPHA:
			return GL_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DST:
			return GL_DST_COLOR;
		case BlendFactor::ONE_MINUS_DST:
			return GL_ONE_MINUS_DST_COLOR;
		case BlendFactor::DST_ALPHA:
			return GL_DST_ALPHA;
		case BlendFactor::ONE_MINUS_DST_ALPHA:
			return GL_ONE_MINUS_DST_ALPHA;
		case BlendFactor::SRC_ALPHA_SATURATED:
			return GL_SRC_ALPHA_SATURATE;
		default:
			BX_FAIL("Blend factor not supported.");
			return GL_INVALID_ENUM;
		}
	}

	GLenum PrimitiveTopologyToGl(const PrimitiveTopology& topology)
	{
		switch (topology)
		{
		case PrimitiveTopology::POINT_LIST:
			return GL_POINTS;
		case PrimitiveTopology::LINE_LIST:
			return GL_LINES;
		case PrimitiveTopology::LINE_STRIP:
			return GL_LINE_STRIP;
		case PrimitiveTopology::TRIANGLE_LIST:
			return GL_TRIANGLES;
		case PrimitiveTopology::TRIANGLE_STRIP:
			return GL_TRIANGLE_STRIP;
		default:
			BX_FAIL("Primitive topology not supported.");
			return GL_INVALID_ENUM;
		}
	}

	GLenum IndexFormatToGl(const IndexFormat& format)
	{
		switch (format)
		{
		case IndexFormat::UINT16:
			return GL_UNSIGNED_SHORT;
		case IndexFormat::UINT32:
			return GL_UNSIGNED_INT;
		default:
			BX_FAIL("Index format not supported.");
			return GL_INVALID_ENUM;
		}
	}

	GLint VertexFormatToGlSize(const VertexFormat& format)
	{
		switch (format)
		{
		case VertexFormat::UINT_8X2:
			return 2;
		case VertexFormat::UINT_8X4:
			return 4;
		case VertexFormat::SINT_8X2:
			return 2;
		case VertexFormat::SINT_8X4:
			return 4;
		case VertexFormat::UNORM_8X2:
			return 2;
		case VertexFormat::UNORM_8X4:
			return 4;
		case VertexFormat::SNORM_8X2:
			return 2;
		case VertexFormat::SNORM_8X4:
			return 4;

		case VertexFormat::UINT_16X2:
			return 2;
		case VertexFormat::UINT_16X4:
			return 4;
		case VertexFormat::SINT_16X2:
			return 2;
		case VertexFormat::SINT_16X4:
			return 4;
		case VertexFormat::UNORM_16X2:
			return 2;
		case VertexFormat::UNORM_16X4:
			return 4;
		case VertexFormat::SNORM_16X2:
			return 2;
		case VertexFormat::SNORM_16X4:
			return 4;
		case VertexFormat::FLOAT_16X2:
			return 2;
		case VertexFormat::FLOAT_16X4:
			return 4;

		case VertexFormat::FLOAT_32:
			return 1;
		case VertexFormat::FLOAT_32X2:
			return 2;
		case VertexFormat::FLOAT_32X3:
			return 3;
		case VertexFormat::FLOAT_32X4:
			return 4;
		case VertexFormat::UINT_32:
			return 1;
		case VertexFormat::UINT_32X2:
			return 2;
		case VertexFormat::UINT_32X3:
			return 3;
		case VertexFormat::UINT_32X4:
			return 4;
		case VertexFormat::SINT_32:
			return 1;
		case VertexFormat::SINT_32X2:
			return 2;
		case VertexFormat::SINT_32X3:
			return 3;
		case VertexFormat::SINT_32X4:
			return 4;
		default:
			BX_FAIL("Vertex format not supported.");
			return 0;
		}
	}

	GLenum VertexFormatToGlType(const VertexFormat& format)
	{
		switch (format)
		{
		case VertexFormat::UINT_8X2:
			return GL_UNSIGNED_BYTE;
		case VertexFormat::UINT_8X4:
			return GL_UNSIGNED_BYTE;
		case VertexFormat::SINT_8X2:
			return GL_BYTE;
		case VertexFormat::SINT_8X4:
			return GL_BYTE;
		case VertexFormat::UNORM_8X2:
			return GL_UNSIGNED_BYTE;
		case VertexFormat::UNORM_8X4:
			return GL_UNSIGNED_BYTE;
		case VertexFormat::SNORM_8X2:
			return GL_BYTE;
		case VertexFormat::SNORM_8X4:
			return GL_BYTE;

		case VertexFormat::UINT_16X2:
			return GL_UNSIGNED_SHORT;
		case VertexFormat::UINT_16X4:
			return GL_UNSIGNED_SHORT;
		case VertexFormat::SINT_16X2:
			return GL_SHORT;
		case VertexFormat::SINT_16X4:
			return GL_SHORT;
		case VertexFormat::UNORM_16X2:
			return GL_UNSIGNED_SHORT;
		case VertexFormat::UNORM_16X4:
			return GL_UNSIGNED_SHORT;
		case VertexFormat::SNORM_16X2:
			return GL_SHORT;
		case VertexFormat::SNORM_16X4:
			return GL_SHORT;
		case VertexFormat::FLOAT_16X2:
			return GL_HALF_FLOAT;
		case VertexFormat::FLOAT_16X4:
			return GL_HALF_FLOAT;

		case VertexFormat::FLOAT_32:
			return GL_FLOAT;
		case VertexFormat::FLOAT_32X2:
			return GL_FLOAT;
		case VertexFormat::FLOAT_32X3:
			return GL_FLOAT;
		case VertexFormat::FLOAT_32X4:
			return GL_FLOAT;
		case VertexFormat::UINT_32:
			return GL_UNSIGNED_INT;
		case VertexFormat::UINT_32X2:
			return GL_UNSIGNED_INT;
		case VertexFormat::UINT_32X3:
			return GL_UNSIGNED_INT;
		case VertexFormat::UINT_32X4:
			return GL_UNSIGNED_INT;
		case VertexFormat::SINT_32:
			return GL_INT;
		case VertexFormat::SINT_32X2:
			return GL_INT;
		case VertexFormat::SINT_32X3:
			return GL_INT;
		case VertexFormat::SINT_32X4:
			return GL_INT;
		default:
			BX_FAIL("Vertex format not supported.");
			return GL_INVALID_ENUM;
		}
	}

	GLenum StorageTextureAccessToGl(const StorageTextureAccess& access)
	{
		switch (access)
		{
		case StorageTextureAccess::READ:
			return GL_READ_ONLY;
		case StorageTextureAccess::WRITE:
			return GL_WRITE_ONLY;
		case StorageTextureAccess::READ_WRITE:
			return GL_READ_WRITE;
		default:
			BX_FAIL("Storage texture access not supported.");
			return GL_INVALID_ENUM;
		}
	}
}