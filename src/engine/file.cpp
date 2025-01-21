#include <engine/file.hpp>
#include <engine/log.hpp>

File& File::Get()
{
    static File instance{};
    return instance;
}

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

bool File::WriteText(StringView filename, StringView text)
{
    // Open the file in write mode (creating it if it doesn't exist)
    OutputFileStream ofs(filename.data(), std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
    {
        LOGE(File, "Failed to open file: {}", filename);
        return false;
    }

    ofs.write(text.data(), text.size());
    if (!ofs)
    {
        LOGE(File, "Failed to write to file: {}", filename);
        return false;
    }

    ofs.close();
    return true;
}