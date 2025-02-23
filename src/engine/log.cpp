#include <engine/log.hpp>
#include <engine/hash.hpp>
#include <engine/file.hpp>

#include <iostream>

BX_MODULE_DEFINE(Log)

static StringView GetLogLevelString(const LogLevel level)
{
	switch (level)
	{
	case LogLevel::LOG_DEBUG: return "D";
	case LogLevel::LOG_INFO:return "I";
	case LogLevel::LOG_WARNING:return "W";
	case LogLevel::LOG_ERROR:return "E";
	case LogLevel::LOG_FATAL:return "F";
	default: return "UNKNOWN";
	}
}

void Log::Write(StringView message,
	const StringView channel,
	const LogLevel level,
	StringView file,
	u32 line,
	Function<void()>&& onClick)
{
	static Hash<StringView> svHash;
	static Hash<Thread::id> tidHash;
	static CString<1048576> formattedMessage; // 1MB of static storage, should cover all cases of log string size

	SizeType channelHash = svHash(channel.data());

	m_mutex.lock();
	//++mNumOfEntriesPerSeverity[static_cast<int>(severity)];

	if (std::this_thread::get_id() != m_mainThreadId)
	{
		formattedMessage.format("[{}:{} | {}] {} ({}): {}", GetLogLevelString(level), channel, tidHash(std::this_thread::get_id()), File::Get().GetFilename(file), line, message);
	}
	else
	{
		formattedMessage.format("[{}:{}] {} ({}): {}", GetLogLevelString(level), channel, File::Get().GetFilename(file), line, message);
	}

	auto existingChannel = m_channels.find(channelHash);
	if (existingChannel == m_channels.end())
	{
		Channel newChannel{};
		newChannel.name = channel;
		existingChannel = m_channels.emplace(channelHash, std::move(newChannel)).first;
	}

	m_entries.emplace_back(existingChannel->second, level, file, line, std::move(onClick));
	//mEntryContents->Emplace(formattedMessage);

	m_mutex.unlock();

	//if (mEntryContents->SizeInBytes() > sMaxNumOfBytesStored)
	//{
	//	Clear();
	//	LOG(LogCore, Message, "Log buffer exceeded {} bytes, buffer has been cleared", sMaxNumOfBytesStored);
	//}

#ifdef DEBUG_BUILD
	std::cout << formattedMessage << std::endl;
#else
	if (level >= LogLevel::LOG_WARNING)
		std::cout << formattedMessage << std::endl;
#endif
	
	//if (severity == Fatal)
	//{
	//	DumpToCrashLogAndExit();
	//}
}

void Log::Clear()
{
	//m_mutex.lock();
	m_entries.clear();
	//m_entryContents->Clear();
	//m_NumOfEntriesPerSeverity = {};
	//m_mutex.unlock();
}