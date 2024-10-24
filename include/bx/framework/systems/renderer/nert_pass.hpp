#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class BlasDataPool;
class MaterialPool;
class Sky;
class ReblurPass;
class RestirDiPass;
class TaaPass;

struct NertCreateInfo
{
	TextureHandle colorTarget = TextureHandle::null;
	TextureHandle depthTarget = TextureHandle::null;
	TlasHandle tlas = TlasHandle::null;
};

struct NertDispatchInfo
{
	const Camera& camera;
	const BlasDataPool& blasDataPool;
	const MaterialPool& materialPool;
	const Sky& sky;

	TextureViewHandle gbuffer;
	TextureViewHandle gbufferHistory;
	TextureViewHandle reprojection;
	TextureViewHandle velocityView;
};

class NertPass : NoCopy
{
public:
	NertPass(const NertCreateInfo& createInfo);
	~NertPass();

	TextureViewHandle GetThroughputTextureView() const;
	TextureViewHandle GetNeGbufferTextureView() const;
	TextureViewHandle GetAmbientEmissiveBaseColorTextureView() const;

	void SetTlas(TlasHandle tlas);
	void Dispatch(const NertDispatchInfo& dispatchInfo);
	void NextFrame();

	static void ClearPipelineCache();

	u32 maxBounces = 10;
	u32 seed = 1337;
	u32 accumulationFrameIdx = 0;
	b8 unbiased = false;
	b8 restir = true;
	b8 denoise = true;
	b8 antiFirefly = true;
	f32 lightingUpscaleFactor = 2.0;
	b32 ibl = true;

	Vec3 fogColor = Vec3(0.1, 0.1, 0.1);
	f32 fogDensity = 0.05;

private:
	NertCreateInfo createInfo;
	TextureViewHandle colorTargetView;
	u32 width, height;
	u32 lightingWidth, lightingHeight;
	u32 frameIdx;

	std::unique_ptr<RestirDiPass> restirDiPass;
	std::unique_ptr<ReblurPass> reblurPass;
	std::unique_ptr<TaaPass> taaPass;
	std::unique_ptr<TaaPass> preTaaPass;

	SamplerHandle linearRepeatSampler;

	TextureHandle neGbuffer[2];
	TextureViewHandle neGbufferView[2];

	TextureHandle illuminationTexture;
	TextureViewHandle illuminationTextureView;
	TextureHandle ambientEmissiveBaseColorTexture;
	TextureViewHandle ambientEmissiveBaseColorTextureView;
	TextureHandle throughputTexture;
	TextureViewHandle throughputTextureView;

	BufferHandle raysBuffer;
	BufferHandle identityPixelMappingBuffer;
	BufferHandle sampleCountBuffer;
	BufferHandle samplePixelMappingBuffer;
	BufferHandle intersectionsBuffer;
	BufferHandle indirectArgsBuffer;

	BufferHandle intersectConstantsBuffer;
	BufferHandle raygenConstantsBuffer;
	BufferHandle resolveConstantsBuffer;
	BufferHandle samplegenConstantsBuffer;
	BufferHandle shadeConstantsBuffer;

	void UpdateConstantBuffers(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateIntersectBindGroup(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateRaygenBindGroup(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateResolveBindGroup(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateSamplegenBindGroup(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateShadeBindGroup(const NertDispatchInfo& dispatchInfo);
};