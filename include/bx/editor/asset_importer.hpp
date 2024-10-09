#pragma once

#include <bx/containers/string.hpp>

class AssetImporter
{
public:
	using ImportFn = bool(*)(const char* ext, const char* filename);

	static void RegisterImport(const ImportFn& importFn);
	
	static bool Import(const char* ext, const char* filename);
};