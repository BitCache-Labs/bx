#pragma once

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/math.hpp>
#include <bx/engine/modules/graphics.hpp>

#include "bx/framework/components/camera.hpp"
#include "bx/framework/systems/renderer/wfpt_pass.hpp"

class SceneView;

class Renderer : public System
{
public:
	/*Vec4i GetLightIndices(const Vec3& pos);

	void UpdateAnimators();
	void UpdateCameras();
	void UpdateLights();

	void CollectDrawCommands();

	void DrawCommand(const GraphicsHandle pipeline, u32 numResourceBindings, const GraphicsHandle* pResourcesBindings, u32 numBuffers, const GraphicsHandle* pBuffers, const u64* offset, const GraphicsHandle indexBuffer, u32 count);
	void DrawCommands();

	void BindConstants(const Mat4& viewMtx, const Mat4& projMtx, const Mat4& viewProjMtx);*/

	// Returns the color target texture rendered from the editor camera, will return null handle if not in editor build
	TextureHandle GetEditorCameraColorTarget();

private:
	friend class SceneView;

	void Initialize() override;
	void Shutdown() override;

	void Update() override;
	void Render() override;

	u32 frameIdx = 0;

	TextureHandle m_colorTarget = TextureHandle::null;

	TlasHandle m_tlas = TlasHandle::null;

	b8 m_dirtyPasses = true;
	std::unique_ptr<WfptPass> m_wfptPass = nullptr;

	List<Camera> m_cameras{};
	OptionalView<Camera> m_editorCamera = OptionalView<Camera>::None();

	void UpdateCameras();
	void UpdateTlas();
	void RecreateRenderTargets();
	void RebuildPasses();
};