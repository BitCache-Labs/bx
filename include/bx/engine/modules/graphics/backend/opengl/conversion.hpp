#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"

#include "opengl_api.hpp"
#include "bx/engine/modules/graphics/type.hpp"

namespace Gl
{
	GLenum TextureDimensionToGl(const TextureDimension& dimension, u32 depthOrArrayLayers);
	GLenum TextureFormatToGlType(const TextureFormat& format);
	GLenum TextureFormatToGlFormat(const TextureFormat& format);
	GLenum TextureFormatToGlInternalFormat(const TextureFormat& format);

	GLenum ShaderTypeToGl(const ShaderType& type);

	GLenum BlendFactorToGl(const BlendFactor& factor);

	GLenum PrimitiveTopologyToGl(const PrimitiveTopology& topology);
	GLenum IndexFormatToGl(const IndexFormat& format);

	GLint VertexFormatToGlSize(const VertexFormat& format);
	GLenum VertexFormatToGlType(const VertexFormat& format);

	GLenum StorageTextureAccessToGl(const StorageTextureAccess& access);
}