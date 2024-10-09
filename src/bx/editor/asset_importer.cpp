#include "bx/editor/asset_importer.hpp"

#include <bx/containers/list.hpp>

static List<AssetImporter::ImportFn> g_importers;

void AssetImporter::RegisterImport(const ImportFn& importFn)
{
	g_importers.emplace_back(importFn);
}

bool AssetImporter::Import(const char* ext, const char* filename)
{
	for (const auto& importer : g_importers)
	{
		if (importer(ext, filename))
			return true;
	}
	return false;
}