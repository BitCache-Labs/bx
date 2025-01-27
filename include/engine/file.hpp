#pragma once

#include <engine/api.hpp>
#include <engine/module.hpp>
#include <engine/string.hpp>
#include <engine/list.hpp>
#include <fstream>

using InputFileStream = std::ifstream;
using OutputFileStream = std::ofstream;

class BX_API File
{
    BX_MODULE(File)

public:
    StringView GetFilename(StringView file);
    CString<512> GetPath(StringView filename);
    List<StringView> SplitPath(StringView path, char delimiter = '/');
    bool WriteText(StringView filename, StringView text);
};