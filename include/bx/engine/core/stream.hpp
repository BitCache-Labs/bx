#pragma once

#include "bx/engine/core/byte_types.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/list.hpp"

class InputStringStream
{
public:
    InputStringStream(const String& source);
    bool GetLine(String& line);

private:
    const String& m_source;
    SizeType m_length;
    SizeType m_currentPos;
};

class OutputStringStream
{
public:
    OutputStringStream& operator<<(const String& str);
    OutputStringStream& operator<<(const char* str);
    OutputStringStream& operator<<(char c);
    OutputStringStream& operator<<(int value);
    OutputStringStream& operator<<(float value);
    OutputStringStream& operator<<(double value);

    String GetString() const;

private:
    List<String> m_buffer;
};