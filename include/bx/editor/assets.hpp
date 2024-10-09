#pragma once

#include "bx/engine/resource.hpp"

#include <bx/containers/string.hpp>
#include <bx/containers/tree.hpp>

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