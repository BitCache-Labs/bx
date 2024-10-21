#pragma once

#include <bx/bx.hpp>
#include <bx/core/file.hpp>
#include <bx/core/thread.hpp>
#include <bx/containers/list.hpp>
#include <bx/containers/string.hpp>
#include <bx/containers/hash_map.hpp>
#include <bx/containers/tree.hpp>
#include <bx/engine/resource.hpp>

using AssetEditorFn = class AssetEditor*(*)();
using AssetImportFn = bool(*)(const char* ext, const char* filename);

struct BX_API Asset
{
	String path;
	String name;
	String extension;
	bool isDirectory = false;
};

namespace std
{
	template <>
	struct hash<Asset>
	{
		inline std::size_t operator()(const Asset& e) const
		{
			static std::hash<String> hashFn;
			return hashFn(e.path);
		}
	};
}

class BX_API AssetsManager
{
public:
	static AssetsManager& Get();

public:
	bool Initialize();
	void Reload();
	void Shutdown();

	void Refresh();
	const Tree<Asset>& GetAssetTree();
	TreeNodeId GetAssetRootId();

	void RegisterEditor(TypeId typeId, const AssetEditorFn& editorFn);
	void OpenEditor(TypeId typeId);

	void RegisterImport(const AssetImportFn& importFn);
	bool Import(const char* ext, const char* filename);

private:
	static void RefreshTask(Tree<Asset>& assetTree, AssetsManager& ctx);

private:
	Tree<Asset> m_assetTree;
	Tree<Asset> m_assetTreeCopy;
	TreeNodeId m_assetRoot;

	// Define a mutex and condition variable for synchronization
	std::thread m_refreshThread;
	std::mutex m_mtx;
	bool m_ready = false;
	bool m_processed = false;
	bool m_exit = false;

private:
	HashMap<TypeId, AssetEditorFn> m_editors;
	List<AssetImportFn> m_importers;
};