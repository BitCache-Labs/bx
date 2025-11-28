#ifndef BXL_HPP
#define BXL_HPP

#include <bx.hpp>
#include <fmt/format.h>

namespace bx
{
	template<typename... Args>
	inline void logf(log_t level, cstring fstr, Args&&... args) bx_noexcept
	{
		auto str = fmt::format(fstr, std::forward<Args>(args)...);
		log(level, str.c_str());
	}

	template<typename... Args>
	inline void logf_v(log_t level, category_t category, cstring func, cstring file, i32 line, cstring fstr, Args&&... args) bx_noexcept
	{
		auto str = fmt::format(fstr, std::forward<Args>(args)...);
		log_v(level, category, func, file, line, str.c_str());
	}
}

#endif //BXL_HPP
