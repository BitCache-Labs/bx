#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

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

	void Dispatch(const Camera& camera);

	static void ClearPipelineCache();

	u32 maxBounces = 3;
	u32 seed = 1337;

private:
	WfptCreateInfo createInfo;
	TextureViewHandle colorTargetView;
	u32 width, height;

	BufferHandle raysBuffer[2];
	BufferHandle pixelMappingBuffer[2];
	BufferHandle rayCountBuffer[2];
	BufferHandle shadowRaysBuffer;
	BufferHandle shadowRayDistancesBuffer;
	BufferHandle shadowRayCountBuffer;
	BufferHandle shadowRayPixelMappingBuffer;
	BufferHandle intersectionsBuffer;
	BufferHandle payloadsBuffer;

	BufferHandle raygenConstantsBuffer;
	BufferHandle resolveConstantsBuffer;

	BindGroupHandle connectBindGroup;
	BindGroupHandle raygenBindGroup;
	BindGroupHandle resolveBindGroup;
};