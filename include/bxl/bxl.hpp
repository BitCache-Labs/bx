#ifndef BXL_HPP
#define BXL_HPP

#include <bx.hpp>
#include <fmt/format.h>

namespace bx
{
	template<typename... Args>
	inline void logf(log_t level, cstring fstr, Args&&... args)
	{
		auto str = fmt::format(fstr, std::forward<Args>(args)...);
		log(level, str.c_str());
	}

	template<typename... Args>
	inline void logf_v(log_t level, cstring func, cstring file, i32 line, cstring fstr, Args&&... args)
	{
		auto str = fmt::format(fstr, std::forward<Args>(args)...);
		log_v(level, func, file, line, str.c_str());
	}
}

#endif //BXL_HPP
