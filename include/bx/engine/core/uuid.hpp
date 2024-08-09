#pragma once

#include "bx/engine/core/byte_types.hpp"

using Uuid = u64;

class GenUuid
{
public:
	static Uuid MakeUuid();
};