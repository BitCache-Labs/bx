#include "bx/engine/modules/graphics/backend/vulkan/conversion.hpp"

namespace Vk
{
	VkBufferUsageFlags BufferUsageFlagsToVk(BufferUsageFlags usage)
	{
		VkBufferUsageFlags result{};
		if (usage & BufferUsageFlags::COPY_SRC)
			result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (usage & BufferUsageFlags::COPY_DST)
			result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (usage & BufferUsageFlags::INDEX)
			result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (usage & BufferUsageFlags::VERTEX)
			result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (usage & BufferUsageFlags::UNIFORM)
			result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if (usage & BufferUsageFlags::STORAGE)
			result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (usage & BufferUsageFlags::INDIRECT)
			result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		return result;
	}

	VkImageUsageFlags TextureUsageFlagsToVk(TextureUsageFlags usage, b8 depthFormat)
	{
		VkImageUsageFlags result{};
		if (usage & TextureUsageFlags::COPY_SRC)
			result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (usage & TextureUsageFlags::COPY_DST)
			result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (usage & TextureUsageFlags::TEXTURE_BINDING)
			result |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if (usage & TextureUsageFlags::STORAGE_BINDING)
			result |= VK_IMAGE_USAGE_STORAGE_BIT;
		if (usage & TextureUsageFlags::RENDER_ATTACHMENT)
			result |= depthFormat ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		return result;
	}

	VkFormat TextureFormatToVk(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::R8_UNORM:
			return VK_FORMAT_R8_UNORM;
		case TextureFormat::R8_SNORM:
			return VK_FORMAT_R8_SNORM;
		case TextureFormat::R8_UINT:
			return VK_FORMAT_R8_UINT;
		case TextureFormat::R8_SINT:
			return VK_FORMAT_R8_SINT;

		case TextureFormat::R16_UINT:
			return VK_FORMAT_R16_UINT;
		case TextureFormat::R16_SINT:
			return VK_FORMAT_R16_SINT;
		case TextureFormat::R16_UNORM:
			return VK_FORMAT_R16_UNORM;
		case TextureFormat::R16_SNORM:
			return VK_FORMAT_R16_SNORM;
		case TextureFormat::R16_FLOAT:
			return VK_FORMAT_R16_SFLOAT;
		case TextureFormat::RG8_UNORM:
			return VK_FORMAT_R8G8_UNORM;
		case TextureFormat::RG8_SNORM:
			return VK_FORMAT_R8G8_SNORM;
		case TextureFormat::RG8_UINT:
			return VK_FORMAT_R8G8_UINT;
		case TextureFormat::RG8_SINT:
			return VK_FORMAT_R8G8_SINT;

		case TextureFormat::R32_UINT:
			return VK_FORMAT_R32_UINT;
		case TextureFormat::R32_SINT:
			return VK_FORMAT_R32_SINT;
		case TextureFormat::R32_FLOAT:
			return VK_FORMAT_R32_SFLOAT;
		case TextureFormat::RG16_UINT:
			return VK_FORMAT_R16G16_UINT;
		case TextureFormat::RG16_SINT:
			return VK_FORMAT_R16G16_SINT;
		case TextureFormat::RG16_UNORM:
			return VK_FORMAT_R16G16_UNORM;
		case TextureFormat::RG16_SNORM:
			return VK_FORMAT_R16G16_SNORM;
		case TextureFormat::RG16_FLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case TextureFormat::RGBA8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::RGBA8_UNORM_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case TextureFormat::RGBA8_SNORM:
			return VK_FORMAT_R8G8B8A8_SNORM;
		case TextureFormat::RGBA8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;
		case TextureFormat::RGBA8_SINT:
			return VK_FORMAT_R8G8B8A8_SINT;
		case TextureFormat::BGRA8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case TextureFormat::BGRA8_UNORM_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;

		case TextureFormat::RGB9E5_UFLOAT:
			return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case TextureFormat::RGB10A2_UINT:
			return VK_FORMAT_A2R10G10B10_UINT_PACK32;
		case TextureFormat::RGB10A2_UNORM:
			return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case TextureFormat::RG11B10_FLOAT:
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

		case TextureFormat::RG32_UINT:
			return VK_FORMAT_R32G32_UINT;
		case TextureFormat::RG32_SINT:
			return VK_FORMAT_R32G32_SINT;
		case TextureFormat::RG32_FLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
		case TextureFormat::RGBA16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;
		case TextureFormat::RGBA16_SINT:
			return VK_FORMAT_R16G16B16A16_SINT;
		case TextureFormat::RGBA16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case TextureFormat::RGBA16_SNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;
		case TextureFormat::RGBA16_FLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;

		case TextureFormat::RGBA32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;
		case TextureFormat::RGBA32_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;
		case TextureFormat::RGBA32_FLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case TextureFormat::STENCIL8:
			return VK_FORMAT_S8_UINT;
		case TextureFormat::DEPTH16_UNORM:
			return VK_FORMAT_D16_UNORM;
		case TextureFormat::DEPTH24_PLUS:
			return VK_FORMAT_D32_SFLOAT;
		case TextureFormat::DEPTH24_PLUS_STENCIL8:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::DEPTH32_FLOAT:
			return VK_FORMAT_D32_SFLOAT;
		case TextureFormat::DEPTH32_FLOAT_STENCIL8:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;

		case TextureFormat::BC1_RGBA_UNORM:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case TextureFormat::BC1_RGBA_UNORM_SRGB:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case TextureFormat::BC2_RGBA_UNORM:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case TextureFormat::BC2_RGBA_UNORM_SRGB:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case TextureFormat::BC3_RGBA_UNORM:
			return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case TextureFormat::BC3_RGBA_UNORM_SRGB:
			return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case TextureFormat::BC4_R_UNORM:
			return VK_FORMAT_BC4_UNORM_BLOCK;
		case TextureFormat::BC4_R_SNORM:
			return VK_FORMAT_BC4_SNORM_BLOCK;
		case TextureFormat::BC5_RG_UNORM:
			return VK_FORMAT_BC5_UNORM_BLOCK;
		case TextureFormat::BC5_RG_SNORM:
			return VK_FORMAT_BC5_SNORM_BLOCK;
		case TextureFormat::BC6H_RGB_UFLOAT:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case TextureFormat::BC6H_RGB_FLOAT:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case TextureFormat::BC7_RGBA_UNORM:
			return VK_FORMAT_BC7_UNORM_BLOCK;
		case TextureFormat::BC7_RGBA_UNORM_SRGB:
			return VK_FORMAT_BC7_SRGB_BLOCK;

		default:
			BX_FAIL("Texture format not supported.");
			return VK_FORMAT_UNDEFINED;
		}
	}

	VkImageType TextureDimensionToVk(TextureDimension dimension)
	{
		switch (dimension)
		{
		case TextureDimension::D1:
			return VK_IMAGE_TYPE_1D;
		case TextureDimension::D2:
			return VK_IMAGE_TYPE_2D;
		case TextureDimension::D3:
			return VK_IMAGE_TYPE_3D;
		default:
			BX_FAIL("Texture dimension not supported");
			return VK_IMAGE_TYPE_MAX_ENUM;
		}
	}
}