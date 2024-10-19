#pragma once

#include <bx/engine/containers/string.hpp>

#include "bx/engine/modules/graphics/type.hpp"

class AssetImporter
{
public:
	static bool ImportTexture(const String& filename, TextureFormat format);
	static bool ImportModel(const String& filename);
};