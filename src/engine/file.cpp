#include <engine/file.hpp>
#include <engine/log.hpp>

BX_MODULE_DEFINE(File)

StringView File::GetFilename(StringView file)
{
    SizeType lastSlash = file.find_last_of("/\\");
    if (lastSlash == std::string::npos)
        return file;

    return file.substr(lastSlash + 1);
}

CString<512> File::GetPath(StringView filename)
{
    CString<512> filepath = GAME_PATH;
    filepath.append(filename.data());
    return filepath;
}

List<StringView> File::SplitPath(StringView path, char delimiter)
{
    List<StringView> parts;
    SizeType start = 0;
    SizeType end;

    while ((end = path.find(delimiter, start)) != String::npos)
    {
        parts.emplace_back(path.substr(start, end - start));
        start = end + 1;
    }

    if (start < path.size())
        parts.emplace_back(path.substr(start));

    return parts;
}

bool File::WriteText(StringView filename, StringView text)
{
    // Open the file in write mode (creating it if it doesn't exist)
    OutputFileStream ofs(filename.data(), std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
    {
        BX_LOGE(File, "Failed to open file: {}", filename);
        return false;
    }

    ofs.write(text.data(), text.size());
    if (!ofs)
    {
        BX_LOGE(File, "Failed to write to file: {}", filename);
        return false;
    }

    ofs.close();
    return true;
}