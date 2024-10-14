#pragma once

#include <bx/bx.hpp>
#include <bx/core/byte_types.hpp>

using UUID = u64;

class BX_API GenUUID
{
public:
	static UUID MakeUUID();
};