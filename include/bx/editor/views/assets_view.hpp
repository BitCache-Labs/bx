#pragma once

#include <bx/bx.hpp>
#include <bx/core/file.hpp>
#include <bx/core/thread.hpp>
#include <bx/containers/list.hpp>
#include <bx/containers/string.hpp>
#include <bx/containers/tree.hpp>
#include <bx/engine/resource.hpp>
#include <bx/engine/imgui.hpp>

#include <bx/editor/view.hpp>

#include <rttr/rttr_enable.h>

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

class BX_API AssetEditor : public View
{
	RTTR_ENABLE(View)

public:
};

using AssetImportFn = bool(*)(const char* ext, const char* filename);

class BX_API AssetsView final : public View
{
	RTTR_ENABLE(View)

public:
	bool Initialize() override;
	void Reload() override;
	void Shutdown() override;

	void Present() override;

private:
	void ShowFileTree(TreeNodeId nodeId);
	void ProcessFolderImport(TreeNodeId id, const Asset& asset);
	void ProcessAssetContextMenu(TreeNodeId id, const Asset& asset);
	void ShowAssetIcon(f32 iconScale, f32 cellSize, TreeNodeId id, const Asset& asset);
	void ShowAssetLabel(TreeNodeId id, const Asset& asset);
	void ShowGridList(const ImGuiTextFilter& filter);

public:
	void Refresh();
	const Tree<Asset>& GetAssetTree();
	TreeNodeId GetAssetRootId();

	void RegisterImport(const AssetImportFn& importFn);
	bool Import(const char* ext, const char* filename);

private:
	static void RefreshTask(Tree<Asset>& assetTree, AssetsView& ctx);

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
	TreeNodeId m_selectedFolder = INVALID_TREENODE_ID;

	TreeNodeId m_fileEditNode = INVALID_TREENODE_ID;
	bool m_fileEditFocused = false;
	String m_fileEditName;

	bool m_showNewAssetPopup = false;

	List<AssetImportFn> m_importers;

private:
	List<AssetEditor> m_openEditors;
};