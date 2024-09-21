#pragma once

#ifndef MEMORY_CUSTOM_CONTAINERS

#include <array>

template <typename TVal, std::size_t Count>
using Array = std::array<TVal, Count>;

#else // MEMORY_CUSTOM_CONTAINERS

#include "Engine/Core/Memory.hpp"

template <typename TVal>
class Array
{
public:
	// Constructor
	explicit Array(SizeType capacity = 100, Allocator& alloc = Memory::DefaultAllocator())
		: m_alloc(alloc)
		, m_count(0)
		, m_capacity(capacity)
		, m_pData(alloc.NewArray<TVal>(capacity))
	{}

	// Destructor
	~Array()
	{
		m_alloc.DeleteArray(m_pData);
		m_pData = nullptr;
		m_capacity = 0;
		m_count = 0;
	}

	// Move constructor
	Array(Array&& other) noexcept
		: m_alloc(other.m_alloc)
		, m_count(other.m_count)
		, m_capacity(other.m_capacity)
		, m_pData(other.m_pData)
	{
		other.m_count = 0;
		other.m_capacity = 0;
		other.m_pData = nullptr;
	}

	// Copy constructor
	Array(const Array& other)
		: m_alloc(other.m_alloc)
		, m_count(other.m_count)
		, m_capacity(other.m_capacity)
		, m_pData(other.m_alloc.NewArray<TVal>(other.m_capacity))
	{
		for (SizeType i = 0; i < m_count; ++i)
		{
			m_pData[i] = other.m_pData[i];
		}
	}

	// Move assignment operator
	Array& operator=(Array&& other) noexcept
	{
		if (this == &other)
			return *this;

		m_alloc.DeleteArray(m_pData);
		m_alloc = other.m_alloc;

		m_count = other.m_count;
		m_capacity = other.m_capacity;
		m_pData = other.m_pData;

		other.m_count = 0;
		other.m_capacity = 0;
		other.m_pData = nullptr;

		return *this;
	}

	// Copy assignment operator
	Array& operator=(const Array& other)
	{
		if (this == &other)
			return *this;

		m_alloc.DeleteArray(m_pData);
		m_alloc = other.m_alloc;

		m_count = other.m_count;
		m_capacity = other.m_capacity;

		m_pData = m_alloc.NewArray<TVal>(m_capacity);
		for (SizeType i = 0; i < m_count; ++i)
		{
			m_pData[i] = other.m_pData[i];
		}

		return *this;
	}

public:
	class Iterator
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

		inline bool operator==(const Iterator& rhs) const
		{
			return m_ptr == rhs.m_ptr;
		}

		inline bool operator!=(const Iterator& rhs) const
		{
			return !(*this == rhs);
		}

		inline TVal& operator*() const
		{
			return *m_ptr;
		}

		inline Iterator& operator++()
		{
			++m_ptr;
			return *this;
		}

		inline Iterator operator++(int)
		{
			Iterator temp(*this);
			++*this;
			return temp;
		}
	};

	inline Iterator begin() const
	{
		return Iterator(m_pData);
	}

	inline Iterator end() const
	{
		return Iterator(m_pData + m_count);
	}

public:
	inline TVal& Get(SizeType index)
	{
		ENGINE_ASSERT(index < m_count);
		return *(m_pData + index);
	}

	inline const TVal& Get(SizeType index) const
	{
		ENGINE_ASSERT(index < m_count);
		return *(m_pData + index);
	}

	inline void Clear()
	{
		for (SizeType i = 0; i < m_count; ++i)
		{
			auto ptr = m_pData + i;
			ptr->~TVal();
		}

		m_count = 0;
	}

	inline TVal& operator[](SizeType index)
	{
		return Get(index);
	}

	inline const TVal& operator[](SizeType index) const
	{
		return Get(index);
	}

	inline SizeType Count() const
	{
		return m_count;
	}

	inline SizeType Capacity() const
	{
		return m_capacity;
	}

	inline TVal* Data()
	{
		return m_pData;
	}

private:
	Allocator& m_alloc;

	SizeType m_count = 0;
	SizeType m_capacity = 0;

	TVal* m_pData = nullptr;
};
#endif