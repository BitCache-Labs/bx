#pragma once

#include <engine/api.hpp>
#include <engine/module.hpp>
#include <engine/string.hpp>
#include <engine/list.hpp>
#include <engine/hash_map.hpp>

#include <fstream>

using InputFileStream = std::ifstream;
using OutputFileStream = std::ofstream;

struct BX_API FileHandle
{
    CString<512> filepath{};
    CString<64> filename{};
    bool isDirectory{ false };
};

using FindEachCallback = std::function<void(const String& path, const String& name)>;

class BX_API File
{
    BX_MODULE(File)

public:
    bool Initialize();
    void Shutdown();

    void AddWildcard(StringView wildcard, StringView value);

    bool Exists(StringView path);
    u64 LastWrite(StringView filename);

    CString<512> GetPath(StringView filename);

    StringView GetExt(StringView filename);
    StringView RemoveExt(StringView filename);

    StringView GetFilename(StringView file);
    List<StringView> SplitPath(StringView path, char delimiter = '/');

    bool Move(StringView oldPath, StringView newPath);
    bool Delete(StringView path);

    bool CreateDirectory(StringView path);

    bool ListFiles(StringView root, List<FileHandle>& files);
    bool Find(StringView root, StringView filename, CString<512>& filepath);
    void FindEach(StringView root, StringView ext, const FindEachCallback& callback);
    
    String ReadText(StringView filename);
    bool WriteText(StringView filename, StringView text);

    List<char> ReadBinary(StringView filename);

private:
    HashMap<CString<64>, CString<512>> m_wildcards{};
};