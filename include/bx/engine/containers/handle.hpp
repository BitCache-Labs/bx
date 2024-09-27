#pragma once

#include "bx/engine/core/type.hpp"

template <typename T>
struct Handle
{
	u64 id;
	static const Handle<T> null;

	b8 operator==(const Handle<T>& other) const { return id == other.id; }
	b8 operator!=(const Handle<T>& other) const { return id != other.id; }
	operator b8() const { return id != 0; }
};

template <typename T>
const Handle<T> Handle<T>::null = { 0 };

template <typename T>
struct std::hash<Handle<T>>
{
	std::size_t operator()(const Handle<T>& h) const
	{
		return std::hash<u64>()(h.id);
	}
};

template <typename T>
class HandlePool
{
public:
	HandlePool()
	{
		m_count = 0;
	}

	Handle<T> Create()
	{
		m_count++;
		BX_ASSERT(m_count != 18446744073709551615, "Failed to create handles, pool is emtpy.");

		return Handle<T>{ m_count };
	}

	void Destroy(Handle<T>& handle)
	{
		BX_ENSURE(handle);

		u64 id = handle.id;
		handle.id = 0;
	}

private:
	u64 m_count;
};