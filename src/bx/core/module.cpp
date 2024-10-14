#include <bx/core/module.hpp>

#include <bx/core/file.hpp>
#include <bx/core/macros.hpp>

#include <stdexcept>

bool Module::Load(const String& directory)
{
    List<FileHandle> files;
    if (!File::ListFiles(directory, files))
    {
        throw std::runtime_error("Failed to list files!");
    }

    for (auto file : files)
    {
        auto extPos = file.filepath.find(".dll");
        if (extPos != std::string::npos)
        {
            auto path = File::GetPath(file.filepath.substr(0, extPos));
            rttr::library lib(path);
            if (lib.load())
            {
                BX_LOGD("Successfully loaded library: {}", path);
            }
            else
            {
                BX_LOGE("Failed to load library: {} - {}", path, lib.get_error_string().data());
            }
        }
    }
}