#pragma once

#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <iostream>

class StringView
{
public:
    using size_type = std::size_t;
    using const_pointer = const char*;
    using const_reference = const char&;

    static const size_type npos = -1;

    // Constructors
    StringView() noexcept : m_data(nullptr), m_size(0) {}

    StringView(const char* str)
        : m_data(str)
        , m_size(GetCStrLength(str))
    {}

    StringView(const char* str, size_type len)
        : m_data(str)
        , m_size(len)
    {}

    // Element access
    const_reference operator[](size_type pos) const { return m_data[pos]; }
    const_reference At(size_type pos) const
    {
        if (pos >= m_size)
        {
            throw std::out_of_range("StringView::At: position out of range");
        }
        return m_data[pos];
    }

    const_pointer Data() const noexcept { return m_data; }
    size_type Size() const noexcept { return m_size; }
    bool Empty() const noexcept { return m_size == 0; }

    // Substring
    StringView Substr(size_type pos = 0, size_type len = npos) const
    {
        if (pos > m_size)
        {
            throw std::out_of_range("StringView::substr: position out of range");
        }
        return StringView(m_data + pos, std::min(len, m_size - pos));
    }

    // Comparison
    int Compare(StringView other) const
    {
        size_type len = std::min(m_size, other.m_size);
        int result = std::char_traits<char>::compare(m_data, other.m_data, len);
        if (result != 0) return result;
        if (m_size < other.m_size) return -1;
        if (m_size > other.m_size) return 1;
        return 0;
    }

private:
    const char* m_data;
    size_type m_size;

    static size_type GetCStrLength(const char* str)
    {
        size_type len = 0;
        while (str[len] != '\0')
        {
            ++len;
        }
        return len;
    }
};

// Overloaded operator<< for printing
std::ostream& operator<<(std::ostream& os, const StringView& sv)
{
    return os.write(sv.data(), sv.size());
}