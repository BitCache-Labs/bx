#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class BlasDataPool;
class MaterialPool;
class Sky;
class RestirDiPass;

struct WfptCreateInfo
{
	TextureHandle colorTarget = TextureHandle::null;
	TlasHandle tlas = TlasHandle::null;
};

class WfptPass : NoCopy
{
public:
	WfptPass(const WfptCreateInfo& createInfo);
	~WfptPass();

	void SetTlas(TlasHandle tlas);
	void Dispatch(const Camera& camera, const BlasDataPool& blasDataPool, const MaterialPool& materialPool, const Sky& sky);

	static void ClearPipelineCache();

	u32 maxBounces = 3;
	u32 seed = 1337;
	u32 accumulationFrameIdx = 0;
	b8 russianRoulette = true;

private:
	WfptCreateInfo createInfo;
	TextureViewHandle colorTargetView;
	u32 width, height;

	std::unique_ptr<RestirDiPass> restirDiPass;

	BufferHandle raysBuffer[2];
	BufferHandle pixelMappingBuffer[2];
	BufferHandle identityPixelMappingBuffer;
	BufferHandle rayCountBuffer[2];
	BufferHandle shadowRayCountBuffer;
	BufferHandle shadowRayPixelMappingBuffer;
	BufferHandle intersectionsBuffer;
	BufferHandle payloadsBuffer;
	BufferHandle indirectArgsBuffer;

	BufferHandle raygenConstantsBuffer;
	BufferHandle resolveConstantsBuffer;

	BindGroupHandle connectBindGroup;
	BindGroupHandle raygenBindGroup;
	BindGroupHandle resolveBindGroup;
};