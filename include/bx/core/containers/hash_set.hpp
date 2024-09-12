#pragma once

#ifndef MEMORY_CUSTOM_CONTAINERS

#include <unordered_set>

template <typename TVal>
using HashSet = std::unordered_set<TVal>;

#else // MEMORY_CUSTOM_CONTAINERS

#include "Engine/Memory.hpp"

template <typename TVal>
class HashSet
{
public:
	using SizeType = std::size_t;

	explicit HashSet(SizeType capacity = 100, Allocator& alloc = Memory::DefaultAllocator())
		: m_alloc(alloc)
		, m_pData(alloc.NewArray<Pair>(capacity))
		, m_capacity(capacity)
		, m_count(0)
	{}

	~HashSet()
	{
		m_alloc.DeleteArray(m_pData);
		m_capacity = 0;
		m_count = 0;
	}

	void Insert(const TVal& val)
	{
		ENGINE_ASSERT(m_count < m_capacity);

		std::hash<TVal> hashFn;
		SizeType hash = hashFn(val);

		SizeType index = hash % m_capacity;
		while (true)
		{
			Pair& pair = *(m_pData + index);
			if (pair.hash == 0)
			{
				pair.hash = hash;
				pair.value = val;

				m_count++;
				return;
			}

			index = (index + 1) % m_capacity;
		}
	}

	bool Contains(const TVal& val)
	{
		std::hash<TVal> hashFn;
		SizeType hash = hashFn(val);

		SizeType index = hash % m_capacity;
		SizeType start = index;
		while (true)
		{
			Pair& pair = *(m_pData + index);
			if (pair.hash == 0)
				return false;

			if (pair.hash == hash)
				return true;

			index = (index + 1) % m_capacity;
			if (index == start)
				return false;
		}
	}

private:
	struct Pair
	{
		SizeType hash = 0;
		TVal value{};
	};

	Allocator& m_alloc;

	Pair* m_pData = nullptr;
	SizeType m_capacity = 0;
	SizeType m_count = 0;
};

#endif