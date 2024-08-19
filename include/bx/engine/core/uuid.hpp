#pragma once

#include "bx/engine/core/byte_types.hpp"

namespace BX
{
	using UUID = u64;
	
	class GenUuid
	{
	public:
		static UUID MakeUuid();
	};
}