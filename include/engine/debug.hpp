#pragma once

#if defined(EDITOR_BUILD) || defined(DEBUG_BUILD)
#include <engine/api.hpp>
#include <engine/math.hpp>
#include <engine/module.hpp>
#include <engine/list.hpp>
#include <engine/graphics.hpp>

class BX_API Debug
{
	BX_MODULE(Debug)

public:
	bool Initialize();
	void Shutdown();

	void DrawLine(const Vec3& a, const Vec3& b, u32 color);
	void DrawBox(const Box3& box, u32 color);

	void RenderDraws(const Mat4& viewProj);
	void ClearDraws();

private:
	GraphicsHandle m_vertexShader{ INVALID_GRAPHICS_HANDLE };
	GraphicsHandle m_pixelShader{ INVALID_GRAPHICS_HANDLE };
	GraphicsHandle m_pipeline{ INVALID_GRAPHICS_HANDLE };

	GraphicsHandle m_vertexBuffer{ INVALID_GRAPHICS_HANDLE };

	struct Vertex
	{
		Vec3 position{};
		u32 color{};

		Vertex() {}
		Vertex(const Vec3& p, u32 c)
			: position(p)
			, color(c)
		{}
	};

	List<Vertex> m_vertices{};
};
#endif