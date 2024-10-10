#pragma once

#include <rttr/registration.h>
#include <rttr/library.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class Module
{
public:
    static void Load()
    {
        std::vector<std::string> libraryNames = list_library_names("modules");

        for (const auto& libName : libraryNames)
        {
            rttr::library lib(libName);
            if (lib.load())
            {
                std::cout << "Successfully loaded library: " << libName << std::endl;
            }
            else
            {
                std::cerr << "Failed to load library: " << libName << " - " << lib.get_error_string() << std::endl;
            }
        }
    }

private:
    Module() = delete;

    static std::vector<std::string> list_library_names(const std::string& directory);
};
