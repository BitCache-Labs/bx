#pragma once

#include "bx/editor/assets.hpp"

class AssetsView
{
public:
	static void Initialize();
	static void Shutdown();

	static void Present(bool& show);

	struct Callback
	{
		using OnAssetDoubleClickFn = bool(*)(const Asset& asset, bool consumed);
		using OnAssetTargetDropFn = bool(*)(bool consumed);

		OnAssetDoubleClickFn OnAssetDoubleClick = nullptr;
		OnAssetTargetDropFn OnAssetTargetDrop = nullptr;
	};

	static void AddCallback(const Callback& callback);
};