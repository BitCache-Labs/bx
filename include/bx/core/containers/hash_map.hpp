#pragma once

#ifndef MEMORY_CUSTOM_CONTAINERS

#include "bx/engine/core/hash.hpp"

#include <unordered_map>

template <typename TKey, typename TVal>
using HashMap = std::unordered_map<TKey, TVal>;

#else // MEMORY_CUSTOM_CONTAINERS

#include "Engine/Core/Macros.hpp"
#include "Engine/Core/Memory.hpp"
#include "Engine/Core/Hash.hpp"

constexpr SizeType HASHMAP_MAX_ITER_CAPACITY = 1000;
constexpr SizeType HASHMAP_MAX_ITER_COUNT_DIV = 1;
constexpr SizeType HASHMAP_MAX_ITER_COLLS_DIV = 2;
constexpr SizeType HASHMAP_MAX_COUNT_DIV = 2;
constexpr SizeType HASHMAP_MAX_COLLS_DIV = 4;
constexpr SizeType HASHMAP_CAPACITY_INCREMENT_FN(SizeType capacity) { return capacity + capacity; }

constexpr SizeType HASHMAP_INVALID_HASH = 0;

template <typename TKey, typename TVal>
struct HashMapPair
{
	SizeType hash = HASHMAP_INVALID_HASH;

	TKey key{};
	TVal value{};
};

template <typename TKey, typename TVal>
class HashMap
{
public:
	using key_type = TKey;
	using value_type = TVal;

public:
	// Constructor
	explicit HashMap(SizeType capacity = 100, Allocator& alloc = Memory::DefaultAllocator())
		: m_alloc(alloc)
		, m_count(0)
		, m_capacity(capacity)
		, m_collisions(0)
		, m_pData(alloc.NewArray<HashMapPair<TKey, TVal>>(capacity))
	{
		for (SizeType i = 0; i < m_capacity; ++i)
		{
			m_pData[i].hash = HASHMAP_INVALID_HASH;
		}
	}

	// Destructor
	~HashMap()
	{
		m_count = 0;
		m_capacity = 0;
		m_collisions = 0;

		m_alloc.DeleteArray(m_pData);
		m_pData = nullptr;
	}

	// Move constructor
	HashMap(HashMap&& other) noexcept
		: m_alloc(other.m_alloc)
		, m_count(other.m_count)
		, m_capacity(other.m_capacity)
		, m_collisions(other.m_collisions)
		, m_pData(other.m_pData)
	{
		other.m_count = 0;
		other.m_capacity = 0;
		other.m_collisions = 0;
		other.m_pData = nullptr;
	}

	// Copy constructor
	HashMap(const HashMap& other)
		: m_alloc(other.m_alloc)
		, m_count(other.m_count)
		, m_capacity(other.m_capacity)
		, m_collisions(other.m_collisions)
		, m_pData(other.m_alloc.NewArray<HashMapPair<TKey, TVal>>(other.m_capacity))
	{
		for (SizeType i = 0; i < m_capacity; ++i)
		{
			if (other.m_pData[i].hash == HASHMAP_INVALID_HASH)
				continue;

			m_pData[i] = other.m_pData[i];
		}
	}

	// Move assignment operator
	HashMap& operator=(HashMap&& other) noexcept
	{
		if (this == &other)
			return *this;

		m_alloc.DeleteArray(m_pData);
		m_alloc = other.m_alloc;

		m_count = other.m_count;
		m_capacity = other.m_capacity;
		m_collisions = other.m_collisions;
		m_pData = other.m_pData;

		other.m_count = 0;
		other.m_capacity = 0;
		other.m_collisions = 0;
		other.m_pData = nullptr;

		return *this;
	}

	// Copy assignment operator
	HashMap& operator=(const HashMap& other)
	{
		if (this == &other)
			return *this;

		m_alloc.DeleteArray(m_pData);
		m_alloc = other.m_alloc;

		m_count = other.m_count;
		m_capacity = other.m_capacity;
		m_collisions = other.m_collisions;
		
		m_pData = m_alloc.NewArray<HashMapPair<TKey, TVal>>(m_capacity);
		for (SizeType i = 0; i < m_capacity; ++i)
		{
			if (other.m_pData[i].hash == HASHMAP_INVALID_HASH)
				continue;

			m_pData[i] = other.m_pData[i];
		}

		return *this;
	}

public:
	class Iterator
	{
	private:
		const HashMap<TKey, TVal>* m_pMap;
		SizeType m_current;

	public:
		explicit Iterator()
			: m_pMap(nullptr)
			, m_current(0)
		{
		}

		explicit Iterator(const HashMap<TKey, TVal>* pMap, SizeType index)
			: m_pMap(pMap)
			, m_current(index)
		{
			while (m_current < m_pMap->m_capacity &&
				m_pMap->At(m_current)->hash == HASHMAP_INVALID_HASH)
			{
				++m_current;
			}
		}

		inline bool operator==(const Iterator& rhs) const
		{
			return m_pMap == rhs.m_pMap && m_current == rhs.m_current;
		}

		inline bool operator!=(const Iterator& rhs) const
		{
			return !(*this == rhs);
		}

		inline HashMapPair<TKey, TVal>& operator*() const
		{
			return *m_pMap->At(m_current);
		}

		inline HashMapPair<TKey, TVal>* operator->() const
		{
			return m_pMap->At(m_current);
		}

		inline Iterator& operator++()
		{
			++m_current;
			while (m_current < m_pMap->m_capacity &&
				m_pMap->At(m_current)->hash == HASHMAP_INVALID_HASH)
			{
				++m_current;
			}

			return *this;
		}

		inline Iterator operator++(int)
		{
			Iterator temp(*this);
			++*this;
			return temp;
		}

		inline Iterator& operator--()
		{
			++m_current;
			while (m_current < m_pMap->m_capacity &&
				m_pMap->At(m_current)->hash == HASHMAP_INVALID_HASH)
			{
				--m_current;
			}

			return *this;
		}

		inline Iterator operator--(int)
		{
			Iterator temp(*this);
			--*this;
			return temp;
		}

		inline SizeType GetIndex() const { return m_current; }
	};

	inline Iterator begin() const
	{
		return Iterator(this, 0);
	}

	inline Iterator end() const
	{
		return Iterator(this, m_capacity);
	}

public:
	inline Iterator Find(const TKey& key)
	{
		SizeType hash = HashFn(key);
		SizeType index = IndexFn(hash, m_capacity);
		SizeType start = index;
		while (true)
		{
			const auto& pair = *At(index);
			if (pair.hash == HASHMAP_INVALID_HASH)
				return end();

			if (pair.hash == hash)
				return Iterator(this, index);

			index = NextFn(index, m_capacity);
			if (index == start)
				return end();
		}
	}

	inline Iterator Insert(const TKey& key, const TVal& val)
	{
		// As long as capacity is under iterable size
		if (m_capacity <= HASHMAP_MAX_ITER_CAPACITY)
		{
			if (m_count >= m_capacity / HASHMAP_MAX_ITER_COUNT_DIV ||			// We increment the capacity if count is HASHMAP_MAX_ITER_COUNT_DIV full
				m_collisions >= m_capacity / HASHMAP_MAX_ITER_COLLS_DIV)		// Or if collisions is at HASHMAP_MAX_ITER_COLLS_DIV capacity
			{
				Reserve(HASHMAP_CAPACITY_INCREMENT_FN(m_capacity));
			}
		}
		// Otherwise we need to mitigate many iterations
		else
		{
			if (m_count >= m_capacity / HASHMAP_MAX_COUNT_DIV ||		// We increment the capacity if count is HASHMAP_MAX_COUNT_DIV full
				m_collisions >= m_capacity / HASHMAP_MAX_COLLS_DIV)		// Or if collisions is at HASHMAP_MAX_COLLS_DIV of capacity
			{
				Reserve(HASHMAP_CAPACITY_INCREMENT_FN(m_capacity));
			}
		}

		SizeType hash = HashFn(key);
		SizeType index = IndexFn(hash, m_capacity);
		while (At(index)->hash != HASHMAP_INVALID_HASH)
		{
			auto curr = At(index);
			ENGINE_ASSERT(curr->hash != hash, "Entry with the same hash exists in map!");

			index = NextFn(index, m_capacity);
			m_collisions++;
		}
		
		auto* entry = At(index);
		new (entry) HashMapPair<TKey, TVal>{ hash, key, val };

		m_count++;
		return Iterator(this, index);
	}

	inline void Remove(Iterator it)
	{
		ENGINE_ENSURE(it != end());

		it->hash = HASHMAP_INVALID_HASH;
		(&it->key)->~TKey();
		(&it->value)->~TVal();

		Reorganize(it.GetIndex());

		m_count--;
	}

	inline bool Contains(const TKey& key)
	{
		return Find(key) != end();
	}

	inline TVal& Get(const TKey& key)
	{
		auto& it = Find(key);
		ENGINE_ENSURE(it != end());
		return it->value;
	}

	inline void Reserve(SizeType newCapacity)
	{
		if (newCapacity <= m_capacity)
			return;

		m_collisions = 0;

		auto pNew = m_alloc.NewArray<HashMapPair<TKey, TVal>>(newCapacity);
		for (SizeType i = 0; i < m_capacity; ++i)
		{
			auto pair = m_pData[i];
			if (pair.hash == HASHMAP_INVALID_HASH)
				continue;

			pair.hash = HashFn(pair.key);
			SizeType index = IndexFn(pair.hash, newCapacity);
			while (pNew[index].hash != HASHMAP_INVALID_HASH)
			{
				index = NextFn(index, newCapacity);
				m_collisions++;
			}

			pNew[index] = pair;
		}

		m_alloc.DeleteArray(m_pData);
		m_pData = pNew;
		m_capacity = newCapacity;
	}

	inline void Clear()
	{
		for (SizeType i = 0; i < m_capacity; ++i)
		{
			auto pair = m_pData + i;
			if (pair->hash == HASHMAP_INVALID_HASH)
				continue;

			pair->hash = HASHMAP_INVALID_HASH;
			(&pair->key)->~TKey();
			(&pair->value)->~TVal();
		}
	}

	inline TVal& operator[](const TKey& key)
	{
		auto it = Find(key);
		if (it == end())
		{
			it = Insert(key, TVal{});
			return it->value;
		}
		return it->value;
	}

	inline const TVal& operator[](const TKey& key) const
	{
		return Get(key);
	}

	inline SizeType Count() const
	{
		return m_count;
	}

	inline SizeType Capacity() const
	{
		return m_capacity;
	}

	inline HashMapPair<TKey, TVal>* Data()
	{
		return m_pData;
	}

private:
	inline SizeType HashFn(const TKey& key)
	{
		static Hash<TKey> hashFn;
		return hashFn(key);
	}

	inline SizeType IndexFn(const SizeType& hash, const SizeType& capacity) const
	{
		return hash % capacity;
	}

	inline SizeType PrevFn(const SizeType& index, const SizeType& capacity) const
	{
		return (index - 1) % capacity;
	}

	inline SizeType NextFn(const SizeType& index, const SizeType& capacity) const
	{
		return (index + 1) % capacity;
	}

	inline void Reorganize(SizeType index)
	{
		SizeType next = NextFn(index, m_capacity);

		if (At(next)->hash == HASHMAP_INVALID_HASH)
			return;

		if (IndexFn(At(next)->hash, m_capacity) != index)
			return;

		while (IndexFn(At(next)->hash, m_capacity) == index)
		{
			*At(PrevFn(next, m_capacity)) = *At(next);
			next = NextFn(next, m_capacity);

			if (At(next)->hash == HASHMAP_INVALID_HASH)
				break;
		}

		Reorganize(next);
	}

	inline HashMapPair<TKey, TVal>* At(const SizeType& index) const
	{
		ENGINE_ENSURE(index < m_capacity);
		return m_pData + index;
	}

private:
	Allocator& m_alloc;
		
	SizeType m_count = 0;
	SizeType m_capacity = 0;

	SizeType m_collisions = 0;

	HashMapPair<TKey, TVal>* m_pData = nullptr;
};

#endif