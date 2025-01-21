#pragma once

#include <engine/guard.hpp>
#include <engine/string.hpp>
#include <fstream>

using InputFileStream = std::ifstream;
using OutputFileStream = std::ofstream;

class File
{
    SINGLETON(File)

public:
    static File& Get();

    StringView GetFilename(StringView file);
    CString<512> GetPath(StringView filename);
    bool WriteText(StringView filename, StringView text);
};