#pragma once

#include <bx/engine/core/resource.hpp>
#include <bx/engine/containers/string.hpp>
#include <bx/engine/containers/tree.hpp>

struct Asset
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

class AssetManager
{
public:
	static void Initialize();
	static void Shutdown();

	static void Refresh();

    static const Tree<Asset>& GetAssetTree();
    static TreeNodeId GetAssetRootId();
};