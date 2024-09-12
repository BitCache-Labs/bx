#include "bx/engine/core/object.hpp"

#include "bx/engine/core/memory.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/hash_map.hpp"

#include <unordered_map>

struct ObjectInfo
{
	void* pStorage = nullptr;
	SizeType refCount = 0;
	TypeId typeId = INVALID_TYPEID;
};

static HashMap<ObjectHandle, ObjectInfo> s_objects;

ObjectHandle ObjectRef::NewObj(TypeId typeId, SizeType size, SizeType align)
{
	static ObjectHandle s_handleCounter = 0;

#ifdef MEMORY_CUSTOM_CONTAINERS
	void* ptr = Memory::DefaultAllocator().Allocate(size, align);
#else
	void* ptr = std::malloc(size);
#endif
	auto handle = ++s_handleCounter;

	ObjectInfo info;
	info.pStorage = ptr;
	info.refCount = 1;
	info.typeId = typeId;

	s_objects.insert(std::make_pair(handle, info));

	return handle;
}

void ObjectRef::DeleteObj(ObjectHandle handle)
{
	auto it = s_objects.find(handle);
	BX_ENSURE(it != s_objects.end());

#ifdef MEMORY_CUSTOM_CONTAINERS
	Memory::Deallocate(it->second.pStorage);
#else
	std::free(it->second.pStorage);
#endif

	s_objects.erase(it);
}

void* ObjectRef::GetObj(ObjectHandle handle)
{
	auto it = s_objects.find(handle);
	BX_ENSURE(it != s_objects.end());
	return it->second.pStorage;
}

SizeType& ObjectRef::GetObjRefCount(ObjectHandle handle)
{
	auto it = s_objects.find(handle);
	BX_ENSURE(it != s_objects.end());
	return it->second.refCount;
}

const TypeId ObjectRef::GetObjTypeId(ObjectHandle handle)
{
	auto it = s_objects.find(handle);
	BX_ENSURE(it != s_objects.end());
	return it->second.typeId;
}