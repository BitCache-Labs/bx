#pragma once

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/math.hpp>
#include <bx/engine/modules/graphics.hpp>

#include "bx/framework/components/camera.hpp"
#include "bx/framework/systems/renderer/wfpt_pass.hpp"
#include "bx/framework/systems/renderer/blas_data_pool.hpp"
#include "bx/framework/systems/renderer/material_pool.hpp"
#include "bx/framework/systems/renderer/sky.hpp"

class SceneView;

class Renderer : public System
{
public:
	// Returns the color target texture rendered from the editor camera, will return null handle if not in editor build
	TextureHandle GetEditorCameraColorTarget();

	b8 accumulate = false;
	b8 hybrid = true;
	b8 unbiased = false;

private:
	friend class SceneView;

	void Initialize() override;
	void Shutdown() override;

	void Update() override;
	void Render() override;

	u32 frameIdx = 0;

	TextureHandle m_colorTarget = TextureHandle::null;
	TextureHandle m_depthTarget = TextureHandle::null;

	TlasHandle m_tlas = TlasHandle::null;

	b8 m_dirtyPasses = true;
	std::unique_ptr<WfptPass> m_wfptPass = nullptr;
	std::unique_ptr<BlasDataPool> m_blasDataPool = nullptr;
	std::unique_ptr<MaterialPool> m_materialPool = nullptr;
	std::unique_ptr<Sky> m_sky = nullptr;

	List<Camera> m_cameras{};
	OptionalView<Camera> m_editorCamera = OptionalView<Camera>::None();

	void UpdateCameras();
	void UpdateTlas();
	void RecreateRenderTargets();
	void RebuildPasses();
};