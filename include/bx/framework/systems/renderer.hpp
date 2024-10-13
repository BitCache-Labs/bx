#pragma once

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/math.hpp>
#include <bx/engine/modules/graphics.hpp>

#include "bx/framework/components/camera.hpp"
#include "bx/framework/systems/renderer/gbuffer_pass.hpp"
#include "bx/framework/systems/renderer/nert_pass.hpp"
#include "bx/framework/systems/renderer/blas_data_pool.hpp"
#include "bx/framework/systems/renderer/taa_pass.hpp"
#include "bx/framework/systems/renderer/material_pool.hpp"
#include "bx/framework/systems/renderer/sky.hpp"
#include "bx/framework/systems/renderer/fsr2_pass.hpp"
#include "bx/framework/systems/renderer/fog_pass.hpp"
#include "bx/framework/systems/renderer/ssao_pass.hpp"
#include "bx/framework/systems/renderer/bloom_pass.hpp"

class SceneView;

struct SsaoInfo
{
	b8 enabled = true;
	f32 intensity = 1.0;
	f32 offset = 0.0;
	f32 radius = 1.4;
};

class Renderer : public System
{
public:
	// Returns the color target texture rendered from the editor camera, will return null handle if not in editor build
	TextureHandle GetEditorCameraColorTarget();

	b8 accumulate = false;
	b8 hybrid = true;
	b8 unbiased = false;
	b8 taa = false;
	b8 fsr2 = true;
	b8 bloom = true;
	b8 restir = true;
	b8 denoise = true;
	b8 antiFirefly = true;

	SsaoInfo ssaoInfo{};

private:
	friend class SceneView;

	void Initialize() override;
	void Shutdown() override;

	void Update() override;
	void Render() override;

	f32 upscaleFactor = 1.1;
	u32 frameIdx = 0;

	TextureHandle m_colorTarget = TextureHandle::null;
	TextureHandle m_depthTarget = TextureHandle::null;

	TlasHandle m_tlas = TlasHandle::null;

	b8 m_dirtyPasses = true;
	std::unique_ptr<GBufferPass> m_gbufferPass = nullptr;
	std::unique_ptr<NertPass> m_nertPass = nullptr;
	std::unique_ptr<TaaPass> m_taaPass = nullptr;
	std::unique_ptr<Fsr2Pass> m_fsr2Pass = nullptr;
	std::unique_ptr<FogPass> m_fogPass = nullptr;
	std::unique_ptr<SsaoPass> m_ssaoPass = nullptr;
	std::unique_ptr<BloomPass> m_bloomPass = nullptr;
	std::unique_ptr<BlasDataPool> m_blasDataPool = nullptr;
	std::unique_ptr<MaterialPool> m_materialPool = nullptr;
	std::unique_ptr<Sky> m_sky = nullptr;

	List<Camera> m_cameras{};
	OptionalView<Camera> m_editorCamera = OptionalView<Camera>::None();

	TextureHandle GetFinalColorTarget();

	void UpdateCameras();
	void UpdateTlas();
	void RecreateRenderTargets();
	void RebuildPasses();
};