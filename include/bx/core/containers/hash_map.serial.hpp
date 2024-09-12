#pragma once

#include "bx/engine/containers/hash_map.hpp"

#ifndef MEMORY_CUSTOM_CONTAINERS

#include <cereal/types/unordered_map.hpp>

#else // MEMORY_CUSTOM_CONTAINERS

#include <cereal/cereal.hpp>

// Based on: <cereal/types/unordered_map.hpp>
namespace cereal
{
    template <class Archive, template <typename...> class Map, typename... Args, typename = typename Map<Args...>::value_type>
    inline void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, Map<Args...> const& map)
    {
        ar(make_size_tag(static_cast<size_type>(map.Capacity())));
        ar(make_size_tag(static_cast<size_type>(map.Count())));

        for (const auto& i : map)
            ar(make_map_item(i.key, i.value));
    }

    template <class Archive, template <typename...> class Map, typename... Args, typename = typename Map<Args...>::value_type>
    inline void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, Map<Args...>& map)
    {
        size_type capacity;
        size_type count;

        ar(make_size_tag(capacity));
        ar(make_size_tag(count));

        map.Clear();
        map.Reserve(capacity);

        for (size_t i = 0; i < count; ++i)
        {
            typename Map<Args...>::key_type key;
            typename Map<Args...>::value_type value;

            ar(make_map_item(key, value));

            map.Insert(key, value);
        }
    }
}

#endif