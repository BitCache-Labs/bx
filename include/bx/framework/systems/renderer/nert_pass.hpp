#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class BlasDataPool;
class MaterialPool;
class Sky;
class RestirDiPass;
class GBufferPass;

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
};

class NertPass : NoCopy
{
public:
	NertPass(const NertCreateInfo& createInfo);
	~NertPass();

	void SetTlas(TlasHandle tlas);
	void Dispatch(const NertDispatchInfo& dispatchInfo);

	static void ClearPipelineCache();

	u32 maxBounces = 10;
	u32 seed = 1337;
	u32 accumulationFrameIdx = 0;
	b8 unbiased = false;

private:
	NertCreateInfo createInfo;
	TextureViewHandle colorTargetView;
	u32 width, height;

	std::unique_ptr<GBufferPass> gbufferPass;
	std::unique_ptr<RestirDiPass> restirDiPass;

	TextureHandle neGbuffer;
	TextureViewHandle neGbufferView;

	BufferHandle raysBuffer;
	BufferHandle identityPixelMappingBuffer;
	BufferHandle sampleCountBuffer;
	BufferHandle samplePixelMappingBuffer;
	BufferHandle inverseSamplePixelMappingBuffer;
	BufferHandle intersectionsBuffer;
	BufferHandle indirectArgsBuffer;

	BufferHandle intersectConstantsBuffer;
	BufferHandle raygenConstantsBuffer;
	BufferHandle samplegenConstantsBuffer;
	BufferHandle shadeConstantsBuffer;

	void UpdateConstantBuffers(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateIntersectBindGroup(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateRaygenBindGroup(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateSamplegenBindGroup(const NertDispatchInfo& dispatchInfo);
	BindGroupHandle CreateShadeBindGroup(const NertDispatchInfo& dispatchInfo);
};