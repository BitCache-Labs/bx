#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class WriteIndirectArgsPass : NoCopy
{
public:
	WriteIndirectArgsPass(u32 groupSize);
	~WriteIndirectArgsPass();

	void Dispatch(BufferHandle indirectArgsBuffer, BufferHandle countBuffer);

	static void ClearPipelineCache();

private:
	BufferHandle constantsBuffer;
};