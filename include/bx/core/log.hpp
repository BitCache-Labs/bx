#pragma once

#include "bx/engine/core/byte_types.hpp"
#include "bx/engine/containers/string.hpp"

// For now we leave fmt, String.hpp should handle formatting before logging
#include <fmt/core.h>

// TODO: Remove String.hpp dependency, this is a fundamental class so minimal dependency is needed
// Favour const char* with containers/allocators in the impl cpp
// Otherwise, one option is the Macros.hpp will have to be more utility than currently and separate assert

enum struct LogLevel { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR };

struct LogEntry
{
	LogEntry() {}
	LogEntry(LogLevel level, const String& message)
		: level(level), message(message) {}
	
	LogLevel level{ LogLevel::LOG_INFO };
	String message;
};

class Log
{
public:
	// TODO: This should be in String.hpp
	template <typename... Args>
	static String Format(const String& format, Args&&... args)
	{
		auto str = fmt::format(format, std::forward<Args>(args)...);
		return str;
	}

	static void Print(LogLevel level, const String& message);
	static void Print(const char* file, i32 line, const char* func, LogLevel level, const String& message);

	static LogEntry* GetLogs(SizeType& count);
	static void Clear();
};