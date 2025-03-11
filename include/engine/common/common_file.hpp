#pragma once

#include <engine/file.hpp>
#include <engine/list.hpp>
#include <engine/hash_map.hpp>

class BX_API CommonFile final
    : public File
{
    BX_MODULE(CommonFile, File)

public:
    bool Initialize() override;
    void Shutdown() override;

    void AddMountPoint(StringView mountName, StringView path) override;

    bool Exists(StringView path) override;
    u64 LastWrite(StringView filename) override;

    FilePath GetPath(StringView filename) override;

    StringView GetExt(StringView filename) override;
    StringView RemoveExt(StringView filename) override;

    StringView GetFilename(StringView file) override;
    List<StringView> SplitPath(StringView path, char delimiter = '/') override;

    bool Move(StringView oldPath, StringView newPath) override;
    bool Delete(StringView path) override;

    bool CreateDirectory(StringView path) override;

    bool ListFiles(StringView root, List<FileHandle>& files) override;
    bool Find(StringView root, StringView filename, FilePath& filepath) override;
    void FindEach(StringView root, StringView ext, const FindEachCallback& callback) override;

    String ReadText(StringView filename) override;
    bool WriteText(StringView filename, StringView text) override;

    List<char> ReadBinary(StringView filename) override;

    OutputFileStream OutputStream(StringView filename) override;
    InputFileStream InputStream(StringView filename) override;

private:
    HashMap<FileName, List<MountPoint>> m_mountPoints{};
};