#pragma once

#ifndef MEMORY_CUSTOM_CONTAINERS

#include <vector>

template <typename TVal>
using List = std::vector<TVal>;

#else // MEMORY_CUSTOM_CONTAINERS

#include "Engine/Core/Macros.hpp"
#include "Engine/Core/Memory.hpp"

template <typename TVal>
class List
{
public:
	// TODO: Follow stl conventions for compatibility
	//using type = TVal;

public:
	// Constructor
	explicit List(SizeType count = 0, SizeType capacity = 100, Allocator& alloc = Memory::DefaultAllocator())
		: m_alloc(alloc)
		, m_count(count)
		, m_capacity(capacity)
		, m_pData(alloc.NewArray<TVal>(capacity))
	{
		for (SizeType i = 0; i < m_count; ++i)
		{
			new (&m_pData[i]) TVal();
		}
	}

	// Destructor
	~List()
	{
		m_alloc.DeleteArray(m_pData);
		m_pData = nullptr;
		m_capacity = 0;
		m_count = 0;
	}

	// Move constructor
	List(List&& other) noexcept
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
	List(const List& other)
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
	List& operator=(List&& other) noexcept
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
	List& operator=(const List& other)
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
	inline void Add(const TVal& obj)
	{
		if (m_count == m_capacity)
			Reserve(m_capacity + m_capacity);

		m_pData[m_count++] = obj;
	}

	inline TVal& Get(SizeType index)
	{
		ENGINE_ENSURE(index < m_count);
		return *(m_pData + index);
	}

	inline const TVal& Get(SizeType index) const
	{
		ENGINE_ENSURE(index < m_count);
		return *(m_pData + index);
	}

	inline void Remove(SizeType index, SizeType count = 1)
	{
		if (index >= m_count)
			return;
		
		if ((index + count) > m_count)
			count = m_count - index;

		for (SizeType i = index; i < index + count; ++i)
		{
			TVal* pObj = m_pData + i;
			pObj->~TVal();
		}

		for (SizeType i = index; i < m_count - count; ++i)
		{
			m_pData[i] = m_pData[i + count];
		}
	}

	inline void Reserve(SizeType newCapacity)
	{
		if (newCapacity <= m_capacity)
			return;

		auto pNew = m_alloc.NewArray<TVal>(newCapacity);
		for (SizeType i = 0; i < m_capacity; ++i)
		{
			auto val = m_pData[i];
			pNew[i] = val;
		}

		m_alloc.DeleteArray(m_pData);
		m_pData = pNew;
		m_capacity = newCapacity;
	}

	inline void Resize(SizeType size)
	{
		if (size > m_capacity)
			Reserve(size);

		for (SizeType i = m_count; i < size; i++)
		{
			TVal* pObj = m_pData + i;
			pObj = new (pObj) TVal();
			m_count++;
		}
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

	inline const TVal* Data() const
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