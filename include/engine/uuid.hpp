#pragma once

#include <engine/api.hpp>
#include <engine/byte_types.hpp>

using UUID = u64;

class BX_API GenUUID
{
public:
	static UUID MakeUUID();
};