#include "bx/engine/core/log.hpp"

#include <fmt/core.h>
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#define LOG_LEVEL_OUTPUT 4

static std::vector<LogEntry> g_entries;

void Log::Print(LogLevel level, const String& message)
{
	std::cout << message << std::endl;

	if (level == LogLevel::LOG_DEBUG)
		return;

	// Output the formatted string to the console
	g_entries.emplace_back(level, message);

	if (g_entries.size() > 1024)
		g_entries.erase(g_entries.begin());
}

void Log::Print(const char* file, i32 line, const char* func, LogLevel level, const String& message)
{
	String str = fmt::format("[{}:{} ({})]\n{}", file, line, func, message);
	Print(level, str);
}

LogEntry* Log::GetLogs(SizeType& count)
{
	count = g_entries.size();
	return g_entries.data();
}

void Log::Clear()
{
	g_entries.clear();
}