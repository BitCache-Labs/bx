#pragma once

#include <bx/bx.hpp>
#include <bx/containers/string.hpp>

#include <rttr/type.h>
#include <rttr/library.h>

class BX_API Module
{
public:
    static bool Load(const String& directory);

public:
    template <typename T>
    static T& GetFirstDerived()
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
};
