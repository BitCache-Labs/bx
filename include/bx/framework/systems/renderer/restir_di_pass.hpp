#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class Restir : NoCopy
{
public:
	struct PackedReservoir
	{
		u32 data;
	};

	struct PackedReservoirData
	{
		u32 sampleDirection;
		f32 hitT;
	};

public:
	static BindGroupLayoutDescriptor GetBindGroupLayout();
	constexpr static u32 BIND_GROUP_SET = 4;
};

class RestirDiPass : NoCopy
{
public:
	RestirDiPass(u32 width, u32 height);
	~RestirDiPass();

	BindGroupHandle CreateBindGroup(ComputePipelineHandle pipeline, b8 flipRestirSamples) const;

	void Dispatch(const Camera& camera, TlasHandle tlas, TextureViewHandle gbufferView, TextureViewHandle gbufferHistoryView);

	static void ClearPipelineCache();

	u32 seed = 1337;
	u32 pixelRadius = 30;
	b8 unbiased = false;

	static constexpr u32 SPATIAL_REUSE_PASSES = 2;

private:
	u32 width, height;

	BufferHandle reservoirsBuffer;
	BufferHandle outReservoirsBuffer;
	BufferHandle reservoirsHistoryBuffer;
	BufferHandle reservoirDataBuffer;
	BufferHandle outReservoirDataBuffer;
	BufferHandle reservoirDataHistoryBuffer;

	BufferHandle spatialReuseConstantsBuffers[SPATIAL_REUSE_PASSES];
	BufferHandle temporalReuseConstantsBuffer;

	BindGroupHandle restirTemporalBindGroup;
	BindGroupHandle restirSpatialBindGroups[SPATIAL_REUSE_PASSES];
};