#pragma once

#include "bx/engine/core/type.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/macros.hpp"

#include <utility>

// Goal for ObjectRef and Object<T> is to serve as complete wrappers for pointers and types.
// Basically every time you allocate memory on the heap in engine, it should (nearly) always return an Object.

// They are similar to what smart pointers provide with the addition of an indirection layer between
// the pointer (where the memory resides) and an abstract interface (ObjectRef) which can be passed around without a type.
// Both these classes are ref counted and will free the memory once all references are deleted.

// Points for improvements:
// - Unit testing.
// - Optimizations (see: http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html).
// - Integration with memory allocators (to control where memory resides).
// - Unique ownership (akin to unique_ptr)

// Extra ideas, since the Object wrapper is meant to be a complete solution to raw pointers, the u64 value can be
// interpreted in a few ways:
// - A raw pointer, when performance is critical and the overhead of a lookup is high (should be used with caution)
// - An ID, simply put the value represents an number ID (with a few less bits for the "type" mask)
// - A handle, similar to bitsquid post a handle is used in conjunction with a container.
// - A virtual address, this is the generic case aka a pointer replacement current idea is 32 bits are used as an allocation ID
//   the other 32 bits are used as an byte offset. This interpretation can only be used with engine allocators, each allocator's
//   Allocate function returns an "AllocInfo" type, which includes the allocation ID. This ID can be reused when the more memory
//   needs to be allocated but the u64 value needs to remain the same, the global memory manager will keep track where the alloc
//   ID points in memory so no dangling pointers will exist.
// 
// It's very possible the last interpretation can replace all instances of raw pointers at the cost of a lookup, however,
// it does work as a complete replacement.

// Another thing to consider is being able to dereference the Object so the raw address can be passed around when access to the
// memory is very frequent, maybe a wrapper that can hold the raw pointer and halt any destruction from the parent Object?

using ObjectHandle = u64;
constexpr ObjectHandle INVALID_OBJECT_HANDLE = 0;

template <typename T>
class Object;

class ObjectRef
{
public:
	static ObjectRef Invalid() { return ObjectRef(); }

	ObjectRef() : m_handle(INVALID_OBJECT_HANDLE) {}

	explicit ObjectRef(ObjectHandle handle)
		: m_handle(handle)
	{}

	virtual ~ObjectRef() {}

	// Move constructor
	ObjectRef(ObjectRef&& other) noexcept
		: m_handle(other.m_handle)
	{
		other.m_handle = INVALID_OBJECT_HANDLE;
	}

	// Copy constructor
	ObjectRef(const ObjectRef& other)
		: m_handle(other.m_handle)
	{
		if (m_handle != INVALID_OBJECT_HANDLE)
		{
			GetObjRefCount(m_handle)++;
		}
	}

	// Move assignment operator
	ObjectRef& operator=(ObjectRef&& other) noexcept
	{
		m_handle = other.m_handle;
		other.m_handle = INVALID_OBJECT_HANDLE;
		return *this;
	}

	// Copy assignment operator
	ObjectRef& operator=(const ObjectRef& other)
	{
		if (m_handle != INVALID_OBJECT_HANDLE)
		{
			GetObjRefCount(m_handle)--;
		}

		if (m_handle != other.m_handle)
		{
			m_handle = other.m_handle;
			GetObjRefCount(m_handle)++;
		}
		
		return *this;
	}

	inline bool operator==(const ObjectRef& other) const { return m_handle == other.m_handle; }

	inline const ObjectHandle GetHandle() const { return m_handle; }
	inline const TypeId GetTypeId() const { return GetObjTypeId(m_handle); }

	template <typename T>
	inline bool Is() const;

	template <typename T>
	inline Object<T> As() const;

private:
	template <typename T>
	friend class Object;

	template <typename T, typename... Args>
	static ObjectHandle New(Args&&... args);
	
	template <typename T>
	static void Delete(ObjectHandle handle);

	template <typename T>
	static T* Get(ObjectHandle handle);
	
	static ObjectHandle NewObj(TypeId typeId, SizeType size, SizeType align);
	static void DeleteObj(ObjectHandle handle);
	static void* GetObj(ObjectHandle handle);

	static SizeType& GetObjRefCount(ObjectHandle handle);
	static const TypeId GetObjTypeId(ObjectHandle handle);

private:
	ObjectHandle m_handle;
};

template <typename T>
class Object : public ObjectRef
{
public:
	template <typename... Args>
	explicit Object(Args&&... args)
		: ObjectRef(New<T, Args...>(std::forward<Args>(args)...))
	{
		BX_LOGD("New object ({}): {}", Type<T>::ClassName(), m_handle);
	}

	virtual ~Object()
	{
		if (m_handle != INVALID_OBJECT_HANDLE)
		{
			auto refCount = --GetObjRefCount(m_handle);
			if (refCount == 0)
			{
				Delete<T>(m_handle);
				BX_LOGD("Delete object ({}): {}", Type<T>::ClassName(), m_handle);
			}
		}
		m_handle = INVALID_OBJECT_HANDLE;
	}

	Object(ObjectRef&& other) noexcept : ObjectRef(other) {}
	Object(const ObjectRef& other) : ObjectRef(other) {}

	Object(Object&& other) = default;
	Object(const Object& other) = default;
	
	Object& operator=(Object&& other) = default;
	Object& operator=(const Object& other) = default;

	inline bool operator==(const Object& other) const { return m_handle == other.m_handle; }

	inline T* operator->() { return Get<T>(m_handle); }
	inline T& operator*() { return *Get<T>(m_handle); }

	inline const T* operator->() const { return Get<T>(m_handle); }
	inline const T& operator*() const { return *Get<T>(m_handle); }

	inline T* Ptr() { return Get<T>(m_handle); }
	inline const T* Ptr() const { return Get<T>(m_handle); }
};

template <typename T>
inline bool ObjectRef::Is() const
{
	if (m_handle == INVALID_OBJECT_HANDLE)
		return false;

	return GetTypeId() == Type<T>::Id();
}

template <typename T>
inline Object<T> ObjectRef::As() const
{
	return Object<T>(*this);
}

template <typename T, typename... Args>
ObjectHandle ObjectRef::New(Args&&... args)
{
	ObjectHandle handle = NewObj(Type<T>::Id(), sizeof(T), alignof(T));
	auto ptr = new (GetObj(handle)) T(std::forward<Args>(args)...);

	return handle;
}

template <typename T>
void ObjectRef::Delete(ObjectHandle handle)
{
	auto ptr = Get<T>(handle);

	if (ptr != nullptr)
		delete ptr;

	DeleteObj(handle);
}

template <typename T>
T* ObjectRef::Get(ObjectHandle handle)
{
	return static_cast<T*>(GetObj(handle));
}