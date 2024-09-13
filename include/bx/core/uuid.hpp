#pragma once

#include "bx/core/byte_types.hpp"

using UUID = u64;

class GenUUID
{
public:
	static UUID MakeUUID();
};