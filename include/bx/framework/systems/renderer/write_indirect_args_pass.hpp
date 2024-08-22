#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class WriteIndirectArgsPass : NoCopy
{
public:
	WriteIndirectArgsPass(BufferHandle countBuffer, u32 groupSize);
	~WriteIndirectArgsPass();

	BufferHandle Dispatch();

	static void ClearPipelineCache();

private:
	BufferHandle indirectArgsBuffer;
	BufferHandle constantsBuffer;
	BindGroupHandle bindGroup;
};