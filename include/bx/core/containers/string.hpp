#pragma once

#ifndef MEMORY_CUSTOM_CONTAINERS

#include <string>

using String = std::string;

#else // MEMORY_CUSTOM_CONTAINERS

#include "Engine/Core/Memory.hpp"

//using StringView = std::string; // TODO: Implement this

class String
{
public:
	using SizeType = std::size_t;

	explicit String(SizeType capacity = 100, Allocator& alloc = Memory::DefaultAllocator())
		: m_alloc(alloc)
		, m_count(0)
		, m_capacity(capacity)
		, m_pData(alloc.NewArray<char>(capacity))
	{}

	String(const String& other)
		: m_alloc(other.m_alloc)
		, m_count(other.m_count)
		, m_capacity(other.m_capacity)
		, m_pData(other.m_alloc.NewArray<char>(other.m_capacity))
	{
		Memory::Copy(other.m_pData, m_pData, m_capacity);
	}

	String& operator=(const String& other)
	{
		m_alloc.Deallocate(m_pData);
		m_alloc = other.m_alloc;

		m_count = other.m_count;
		m_capacity = other.m_capacity;

		m_pData = m_alloc.NewArray<char>(m_capacity);
		Memory::Copy(other.m_pData, m_pData, m_capacity);

		return *this;
	}

	~String()
	{
		m_alloc.DeleteArray(m_pData);
		m_capacity = 0;
		m_count = 0;
	}

	inline SizeType Count() const
	{
		return m_count;
	}

	inline SizeType Capacity() const
	{
		return m_capacity;
	}

public:
	/*class Iterator
	{
	private:
		TVal* m_ptr;

	public:
		explicit Iterator()
			: m_ptr(nullptr)
		{
		}
		explicit Iterator(TVal* ptr)
			: m_ptr(ptr)
		{
		}
		bool operator==(const Iterator& rhs) const
		{
			return m_ptr == rhs.m_ptr;
		}
		bool operator!=(const Iterator& rhs) const
		{
			return !(*this == rhs);
		}
		TVal& operator*() const
		{
			return *m_ptr;
		}
		Iterator& operator++()
		{
			++m_ptr;
			return *this;
		}
		Iterator operator++(int)
		{
			Iterator temp(*this);
			++*this;
			return temp;
		}
	};

	Iterator begin() const
	{
		return Iterator(m_pData);
	}

	Iterator end() const
	{
		return Iterator(m_pData + m_count);
	}*/

private:
	Allocator& m_alloc;

	SizeType m_count = 0;
	SizeType m_capacity = 0;

	char* m_pData = nullptr;
};

#endif