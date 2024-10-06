#pragma once

#include "bx/engine/core/byte_types.hpp"

using Uuid = u64;

class GenUUID
{
public:
	static Uuid MakeUUID();
};