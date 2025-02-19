#pragma once

#include <engine/api.hpp>
#include <engine/byte_types.hpp>
#include <engine/macros.hpp>
#include <engine/module.hpp>
#include <engine/memory.hpp>
#include <engine/thread.hpp>
#include <engine/string.hpp>
#include <engine/list.hpp>
#include <engine/hash_map.hpp>
#include <engine/function.hpp>

#include <fmt/core.h>

#define LOG_CHANNEL(Channel) namespace LogChannel { struct BX_API Channel { static StringView Name() { return #Channel; } }; }

LOG_CHANNEL(Log)

#define BX_LOG(Channel, Level, ...) Log::Get().Write(fmt::format(__VA_ARGS__), LogChannel::Channel::Name(), Level, __FILE__, __LINE__)
#define BX_LOGD(Channel, ...) BX_LOG(Channel, LogLevel::LOG_DEBUG, __VA_ARGS__)
#define BX_LOGI(Channel, ...) BX_LOG(Channel, LogLevel::LOG_INFO, __VA_ARGS__)
#define BX_LOGW(Channel, ...) BX_LOG(Channel, LogLevel::LOG_WARNING, __VA_ARGS__)
#define BX_LOGE(Channel, ...) BX_LOG(Channel, LogLevel::LOG_ERROR, __VA_ARGS__)
#define BX_LOGF(Channel, ...) BX_LOG(Channel, LogLevel::LOG_FATAL, __VA_ARGS__)

BX_ENUM_TYPE(LogLevel) : u8 { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARNING = 2, LOG_ERROR = 3, LOG_FATAL = 4, ENUM_COUNT };

struct BX_API LogEntry
{
	LogEntry() {}
	LogEntry(LogLevel level, const String& message)
		: level(level), message(message) {}
	
	LogLevel level{ LogLevel::LOG_INFO };
	String message;
};

// TODO: Rename to Console
class BX_API Log
{
	BX_MODULE(Log)

public:
	void Write(StringView message,
		const StringView channel,
		const LogLevel level,
		StringView file,
		u32 line,
		Function<void()>&& onClick = {});

	void Clear();

private:
	// TODO: Rename to ConsoleEditor
	friend class Console;

	struct Channel
	{
		CString<64> name{};
		bool enabled{ true };

		bool operator<(const Channel& other) const { return name.compare(other.name) < 0; }
	};

	struct Entry
	{
		Entry(const Channel& channel, LogLevel level, StringView file, u32 line, Function<void()>&& onClick)
			: channel(channel)
			, level(level)
			, file(file)
			, line(line)
			, onClick(std::move(onClick))
		{}

		RefWrapper<const Channel> channel;
		LogLevel level;

		StringView file{};
		u32 line{};

		Function<void()> onClick;
	};

	Mutex m_mutex{};
	Thread::id m_mainThreadId{ std::this_thread::get_id() };

	HashMap<u32, Channel> m_channels{};
	List<Entry> m_entries{};
};