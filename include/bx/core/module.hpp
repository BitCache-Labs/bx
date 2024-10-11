#pragma once

#include <bx/bx.hpp>

#include <rttr/registration.h>
#include <rttr/library.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class BX_API Module
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

    template <typename T>
    static T& Get()
    {
        static std::shared_ptr<T> instance;
        if (!instance)
        {
            const auto& derived = rttr::type::get<T>().get_derived_classes();
            if (derived.size() == 0)
                throw std::runtime_error("No derived class found.");

            const auto& type = *derived.begin();
            rttr::variant var = type.create();
            instance = var.get_value<std::shared_ptr<T>>();
        }
        return *instance;
    }

private:
    Module() = delete;

    static std::vector<std::string> list_library_names(const std::string& directory);
};
