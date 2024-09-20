#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class BlasDataPool;
class Sky;

class Restir : NoCopy
{
public:
	struct PackedReservoir
	{
		u32 sampleCountAndContributionWeight;
		f32 weightSum;
	};

	struct PackedReservoirData
	{
		PackedNormalizedXyz10 packedSampleDirection;
		f32 hitT;
		u32 triangleLightSource;
		u32 blasInstance;
		u32 packedHitUv;
		u32 _PADDING0;
		u32 _PADDING1;
		u32 _PADDING2;
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

	void Dispatch(const Camera& camera, TlasHandle tlas, TextureViewHandle gbufferView, TextureViewHandle gbufferHistoryView, const BlasDataPool& blasDataPool, const Sky& sky);

	static void ClearPipelineCache();

	u32 seed = 1337;
	u32 pixelRadius = 30; // TODO: remove
	b8 unbiased = false;
	b8 jacobian = false;

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