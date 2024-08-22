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

private:
	TextureViewHandle colorTargetView;
	u32 width, height;

	BufferHandle raysBuffer;
	BufferHandle rayCountBuffer;
	BufferHandle intersectionsBuffer;

	BufferHandle raygenConstantsBuffer;
	BufferHandle shadeConstantsBuffer;

	BindGroupHandle extendBindGroup;
	BindGroupHandle raygenBindGroup;
	BindGroupHandle shadeBindGroup;
};