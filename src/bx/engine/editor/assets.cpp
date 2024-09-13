#include "bx/editor/core/assets.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/core/thread.hpp>
#include <bx/engine/containers/hash_map.hpp>

static Tree<Asset> g_assetTree;
static Tree<Asset> g_assetTreeCopy;
static TreeNodeId g_assetRoot;

// Define a mutex and condition variable for synchronization
std::thread g_refreshThread;
static std::mutex g_mtx;
static bool g_ready = false;
static bool g_processed = false;
static bool g_exit = false;

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
static void RefreshTask(Tree<Asset>& assetTree)
{
    while (!g_exit)
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
            std::lock_guard<std::mutex> lock(g_mtx);
            g_ready = true;
        }

        // Wait until the data is processed by the main thread
        while (!g_processed && !g_exit)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Reset the data processed flag
        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_processed = false;
        }
    }
}

void AssetManager::Initialize()
{
    Asset root;
    root.path = "[assets]";
    root.name = "Assets";
    root.extension = "";
    root.isDirectory = true;

    g_assetRoot = g_assetTree.CreateNode(root);
    CacheDirectory(g_assetTree, g_assetRoot);

    g_refreshThread = std::thread(RefreshTask, std::ref(g_assetTreeCopy));
}

void AssetManager::Shutdown()
{
    g_exit = true;
    g_refreshThread.join();
}

void AssetManager::Refresh()
{
    bool ready;
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        ready = g_ready;
    }

    if (ready)
    {
        // Lock the mutex to access the shared data
        std::lock_guard<std::mutex> lock(g_mtx);

        // Update asset tree
        g_assetTree = g_assetTreeCopy;

        // Reset the data ready flag
        g_ready = false;

        // Set the data processed flag
        g_processed = true;
    }
}

const Tree<Asset>& AssetManager::GetAssetTree()
{
    return g_assetTree;
}

TreeNodeId AssetManager::GetAssetRootId()
{
    return g_assetRoot;
}