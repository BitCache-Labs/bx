#include <bx/editor/assets.hpp>
#include <bx/editor/views/assets_view.hpp>

AssetsManager& AssetsManager::Get()
{
	static AssetsManager* s_assetsManager = nullptr;
	if (!s_assetsManager) s_assetsManager = new AssetsManager();
	return *s_assetsManager;
}

static void CacheDirectory(Tree<Asset>& assetTree, TreeNodeId nodeIndex)
{
	List<FileHandle> files;
	File::ListFiles(assetTree.GetNode(nodeIndex).data.path, files);
	for (const auto& file : files)
	{
		const auto& node = assetTree.GetNode(nodeIndex);

		Asset child;
		child.path = node.data.path + "/" + file.filename;
		child.name = file.filename;
		child.extension = File::GetExt(child.name);
		child.isDirectory = file.isDirectory;

		TreeNodeId childIndex = assetTree.CreateNode(child);

		assetTree.AddChild(nodeIndex, childIndex);

		if (file.isDirectory)
			CacheDirectory(assetTree, childIndex);
	}
}

// Function to perform some calculation on the map
void AssetsManager::RefreshTask(Tree<Asset>& assetTree, AssetsManager& ctx)
{
	while (!ctx.m_exit)
	{
		// Perform some calculations on the map
		Asset root;
		root.path = "[assets]";
		root.name = "Assets";
		root.extension = "";
		root.isDirectory = true;

		assetTree.Clear();
		auto assetRoot = assetTree.CreateNode(root);
		CacheDirectory(assetTree, assetRoot);

		// Set the data ready flag
		{
			std::lock_guard<std::mutex> lock(ctx.m_mtx);
			ctx.m_ready = true;
		}

		// Wait until the data is processed by the main thread
		while (!ctx.m_processed && !ctx.m_exit)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// Reset the data processed flag
		{
			std::lock_guard<std::mutex> lock(ctx.m_mtx);
			ctx.m_processed = false;
		}
	}
}

bool AssetsManager::Initialize()
{
	Asset root;
	root.path = "[assets]";
	root.name = "Assets";
	root.extension = "";
	root.isDirectory = true;

	m_assetRoot = m_assetTree.CreateNode(root);
	CacheDirectory(m_assetTree, m_assetRoot);

	m_refreshThread = std::thread(RefreshTask, std::ref(m_assetTreeCopy), std::ref(*this));
	
	return true;
}

void AssetsManager::Reload()
{
}

void AssetsManager::Shutdown()
{
	m_exit = true;
	m_refreshThread.join();
}

void AssetsManager::Refresh()
{
	bool ready;
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		ready = m_ready;
	}

	if (ready)
	{
		// Lock the mutex to access the shared data
		std::lock_guard<std::mutex> lock(m_mtx);

		// Update asset tree
		m_assetTree = m_assetTreeCopy;

		// Reset the data ready flag
		m_ready = false;

		// Set the data processed flag
		m_processed = true;
	}
}

const Tree<Asset>& AssetsManager::GetAssetTree()
{
	return m_assetTree;
}

TreeNodeId AssetsManager::GetAssetRootId()
{
	return m_assetRoot;
}

void AssetsManager::RegisterEditor(TypeId typeId, const AssetEditorFn& editorFn)
{
	m_editors.insert(std::make_pair(typeId, editorFn));
}

void AssetsManager::OpenEditor(TypeId typeId)
{
	auto it = m_editors.begin();
	auto view = std::shared_ptr<View>(it->second());
	ViewManager::Get().AddView(view);
}

void AssetsManager::RegisterImport(const AssetImportFn& importFn)
{
	m_importers.emplace_back(importFn);
}

bool AssetsManager::Import(const char* ext, const char* filename)
{
	for (const auto& importer : m_importers)
	{
		if (importer(ext, filename))
			return true;
	}
	return false;
}