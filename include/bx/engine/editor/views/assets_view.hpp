#pragma once

class AssetsView
{
public:
	static void Initialize();
	static void Shutdown();

	static void Present(bool& show);
};