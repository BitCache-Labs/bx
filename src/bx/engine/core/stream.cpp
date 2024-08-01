#include "bx/engine/core/stream.hpp"

InputStringStream::InputStringStream(const String& source)
    : m_source(source), m_length(std::strlen(source.c_str())), m_currentPos(0)
{}

bool InputStringStream::GetLine(String& line)
{
    if (m_currentPos >= m_length)
        return false; // End of string reached

    SizeType start = m_currentPos;
    while (m_currentPos < m_length && m_source[m_currentPos] != '\n')
        ++m_currentPos;

    SizeType end = m_currentPos;
    if (m_currentPos < m_length && m_source[m_currentPos] == '\n')
        ++m_currentPos; // Skip the newline character

    line.assign(m_source.c_str() + start, end - start);
    return true;
}

OutputStringStream& OutputStringStream::operator<<(const String& str)
{
    m_buffer.push_back(str);
    return *this;
}

OutputStringStream& OutputStringStream::operator<<(const char* str)
{
    m_buffer.push_back(std::string(str));
    return *this;
}

OutputStringStream& OutputStringStream::operator<<(char c)
{
    m_buffer.push_back(std::string(1, c));
    return *this;
}

OutputStringStream& OutputStringStream::operator<<(int value)
{
    m_buffer.push_back(std::to_string(value));
    return *this;
}

OutputStringStream& OutputStringStream::operator<<(float value)
{
    m_buffer.push_back(std::to_string(value));
    return *this;
}

OutputStringStream& OutputStringStream::operator<<(double value)
{
    m_buffer.push_back(std::to_string(value));
    return *this;
}

String OutputStringStream::GetString() const
{
    String result;
    result.reserve(m_buffer.size() * 16);  // Reserve a rough estimate of final size to reduce reallocations
    for (const auto& piece : m_buffer)
        result += piece;
    return result;
}