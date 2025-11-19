#ifndef BXL_HPP
#define BXL_HPP

#include <bx.hpp>
#include <fmt/format.h>

namespace bx
{
	template<typename... Args>
	inline void logf(log_t level, category_t category, cstring func, cstring file, i32 line, cstring fstr, Args&&... args) bx_noexcept
	{
		auto str = fmt::format(fstr, std::forward<Args>(args)...);
		log(level, category, func, file, line, str.c_str());
	}
}

#endif //BXL_HPP
