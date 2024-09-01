#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class RestirDiPass : NoCopy
{
public:
	struct RestirPath
	{
		Vec3 x0;
		u32 _PADDING0;
		Vec3 x1;
		u32 _PADDING1;
		Vec3 x2;
		u32 _PADDING2;
	};

	struct RestirSample
	{
		RestirPath path;
		f32 weight;
		u32 _PADDING0;
		u32 _PADDING1;
		u32 _PADDING2;
	};

public:
	RestirDiPass(BufferHandle samplesBuffer, BufferHandle samplesHistoryBuffer);
	~RestirDiPass();

	void Dispatch();

	static void ClearPipelineCache();

private:
	u32 dispatchSize;

	BufferHandle spatialReuseConstantsBuffer;

	BindGroupHandle spatialReuseBindGroup;
	BindGroupHandle restirBindGroup;
};