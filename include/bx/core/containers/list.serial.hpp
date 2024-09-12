#pragma once

#include "bx/engine/containers/list.hpp"

#ifndef MEMORY_CUSTOM_CONTAINERS

#include <cereal/types/vector.hpp>

#else // MEMORY_CUSTOM_CONTAINERS

#include <cereal/cereal.hpp>

namespace cereal
{
    template <class Archive, class T> inline
        typename std::enable_if<traits::is_output_serializable<BinaryData<T>, Archive>::value
        && std::is_arithmetic<T>::value && !std::is_same<T, bool>::value, void>::type
        CEREAL_SAVE_FUNCTION_NAME(Archive& ar, List<T> const& list)
    {
        ar(make_size_tag(static_cast<size_type>(list.Count()))); // number of elements
        ar(binary_data(list.Data(), list.Count() * sizeof(T)));
    }

    template <class Archive, class T> inline
        typename std::enable_if<traits::is_input_serializable<BinaryData<T>, Archive>::value
        && std::is_arithmetic<T>::value && !std::is_same<T, bool>::value, void>::type
        CEREAL_LOAD_FUNCTION_NAME(Archive& ar, List<T>& list)
    {
        size_type listCount;
        ar(make_size_tag(listCount));

        list.Resize(static_cast<SizeType>(listCount));
        ar(binary_data(list.Data(), static_cast<SizeType>(listCount) * sizeof(T)));
    }

    template <class Archive, class T> inline
        typename std::enable_if<(!traits::is_output_serializable<BinaryData<T>, Archive>::value
            || !std::is_arithmetic<T>::value) && !std::is_same<T, bool>::value, void>::type
        CEREAL_SAVE_FUNCTION_NAME(Archive& ar, List<T> const& list)
    {
        ar(make_size_tag(static_cast<size_type>(list.Count()))); // number of elements
        for (auto&& v : list)
            ar(v);
    }

    template <class Archive, class T> inline
        typename std::enable_if<(!traits::is_input_serializable<BinaryData<T>, Archive>::value
            || !std::is_arithmetic<T>::value) && !std::is_same<T, bool>::value, void>::type
        CEREAL_LOAD_FUNCTION_NAME(Archive& ar, List<T>& list)
    {
        size_type size;
        ar(make_size_tag(size));

        list.Resize(static_cast<SizeType>(size));
        for (auto&& v : list)
            ar(v);
    }

    template <class Archive> inline
        void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, List<bool> const& list)
    {
        ar(make_size_tag(static_cast<size_type>(list.Count()))); // number of elements
        for (const auto v : list)
            ar(static_cast<bool>(v));
    }

    template <class Archive> inline
        void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, List<bool>& list)
    {
        size_type size;
        ar(make_size_tag(size));

        list.Resize(static_cast<SizeType>(size));
        for (auto&& v : list)
        {
            bool b;
            ar(b);
            v = b;
        }
    }
}

#endif