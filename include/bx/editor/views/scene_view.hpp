#pragma once

struct ImVec2;

class SceneView
{
public:
	static void Initialize();
	static void Shutdown();

	static void Present(bool& show);

private:
	static void Render(const ImVec2& size);
};