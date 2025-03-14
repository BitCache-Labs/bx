#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/memory.hpp>
#include <engine/guard.hpp>
#include <engine/hash.hpp>

template <typename T>
class Object;

class BX_API ObjectHandle
{
public:
    ObjectHandle() = default; // Default invalid handle

    inline bool IsValid() const { return m_ptr != nullptr; }
    inline operator bool() const { return IsValid(); }

    template <typename T>
    bool Is() const { return m_typeId == Type<T>::Id(); }

    template <typename T>
    Object<T> As() const
    {
        BX_ENSURE(Is<T>()); // Ensure type matches before casting
        return Object<T>(std::static_pointer_cast<T>(m_ptr));
    }

    // Equality & less-than operator for hash map lookups
    inline bool operator==(const ObjectHandle& other) const { return m_ptr == other.m_ptr; }
    inline bool operator<(const ObjectHandle& other) const { return m_ptr < other.m_ptr; }

    inline const void* GetRawPtr() const { return m_ptr.get(); }

protected:
    // ObjectHandle should only be created via Object<T>
    explicit ObjectHandle(TypeId typeId, SharedPtr<void> ptr)
        : m_typeId(typeId)
        , m_ptr(std::move(ptr))
    {}

    TypeId m_typeId{};
    SharedPtr<void> m_ptr{ nullptr };
};

template <typename T>
class BX_API Object : public ObjectHandle
{
public:
    Object() = default;

    template <typename... Args>
    static Object<T> New(Args&&... args)
    {
        auto ptr = Allocate(std::forward<Args>(args)...);
        return Object<T>(ptr);
    }

    inline T* get() const { return std::static_pointer_cast<T>(m_ptr).get(); }
    inline T* operator->() const { return get(); }
    inline T& operator*() const { return *get(); }

private:
    friend class ObjectHandle;

    explicit Object(SharedPtr<T> ptr)
        : ObjectHandle(Type<T>::Id(), ptr)
    {}

    template <typename... Args>
    static SharedPtr<T> Allocate(Args&&... args)
    {
        // TODO: We would like to allocate from an object allocator in the future.
        // The idea is that an object allocator allows for more efficient memory usage.
        // For now we simply allocate on the default heap.
        return meta::make_shared<T>(std::forward<Args>(args)...);
    }
};

template <>
struct BX_API Hash<ObjectHandle>
{
    inline SizeType operator()(const ObjectHandle& v) const
    {
        return v.IsValid() ? HashFunctions::FNV1a(reinterpret_cast<const u8*>(v.GetRawPtr()), sizeof(void*)) : 0;
    }
};