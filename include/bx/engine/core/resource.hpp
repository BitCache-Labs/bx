#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/core/uuid.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/hash_map.hpp"

using ResourceHandle = SizeType;
constexpr ResourceHandle RESOURCE_HANDLE_INVALID = 0;

enum struct ResourceStorage { NONE, DISK, MEMORY };

//class IResource;

template <typename TData>
class Resource;

class ResourceManager
{
public:
	static void Initialize();
	static void Shutdown();

private:
	//friend class IResource;

	static void IncreaseRefCount(ResourceHandle handle);
	static bool DecreaseRefCount(ResourceHandle handle);

private:
	template <typename T>
	friend class Resource;

	static HashMap<ResourceHandle, SizeType>& GetRefCountMap();

};

template <typename TData>
struct ResourceData
{
	ResourceData() {}
	ResourceData(ResourceStorage storage, const String& filename, const TData& data)
		: storage(storage)
		, filename(filename)
		, data(data)
	{}

	SizeType refCount = 0;
	ResourceStorage storage = ResourceStorage::NONE;
	String filename;
	TData data;
};

class IResourceDatabase
{
public:
	IResourceDatabase()
	{
		GetDatabaseRecord().emplace_back(this);
	}

	virtual ~IResourceDatabase() {}

	virtual void Shutdown() = 0;

private:
	friend class ResourceManager;

	static List<IResourceDatabase*>& GetDatabaseRecord()
	{
		static List<IResourceDatabase*> databases;
		return databases;
	}
};

template <typename TData>
class ResourceDatabase : public IResourceDatabase
{
public:
	using LoadFn = bool(*)(const String&, TData&);
	using SaveFn = bool(*)(const String&, const TData&);
	using UnloadFn = void(*)(TData&);

	inline void Shutdown() override
	{
		m_database.clear();
	}

	inline TData& GetData(ResourceHandle handle)
	{
		BX_ASSERT(handle != RESOURCE_HANDLE_INVALID, "Resource handle is invalid!");
		auto it = m_database.find(handle);
		BX_ENSURE(it != m_database.end());
		return it->second.data;
	}

	inline TData* GetDataPtr(ResourceHandle handle)
	{
		BX_ASSERT(handle != RESOURCE_HANDLE_INVALID, "Resource handle is invalid!");
		auto it = m_database.find(handle);
		BX_ENSURE(it != m_database.end());
		return &it->second.data;
	}

	inline const ResourceData<TData>& GetResourceData(ResourceHandle handle)
	{
		BX_ASSERT(handle != RESOURCE_HANDLE_INVALID, "Resource handle is invalid!");
		auto it = m_database.find(handle);
		BX_ENSURE(it != m_database.end());
		return it->second;
	}

	inline bool IsLoaded(ResourceHandle handle)
	{
		return handle != RESOURCE_HANDLE_INVALID
			&& m_database.find(handle) != m_database.end();
	}

	inline bool Load(ResourceHandle handle, const String& filename, LoadFn loadFn)
	{
		BX_ASSERT(handle != RESOURCE_HANDLE_INVALID, "Resource handle is invalid!");
		TData data{};
		
		auto it = m_database.find(handle);
		if (it != m_database.end())
			return true;

		if (!loadFn(filename, data))
		{
			BX_LOGE("Failed to load resource ({} | {})", handle, filename);
			return false;
		}

		auto entry = ResourceData<TData>(ResourceStorage::DISK, filename, data);
		m_database.insert(std::make_pair(handle, entry));

		BX_LOGD("Loaded resource ({} | {})", handle, filename);
		return true;
	}

	inline void LoadData(ResourceHandle handle, const TData& data)
	{
		BX_ASSERT(handle != RESOURCE_HANDLE_INVALID, "Resource handle is invalid!");

		auto it = m_database.find(handle);
		if (it != m_database.end())
			return;

		auto entry = ResourceData<TData>(ResourceStorage::MEMORY, "", data);
		m_database.insert(std::make_pair(handle, entry));

		BX_LOGD("Loaded resource ({})", handle);
	}

	inline void Save(ResourceHandle handle, const String& filename, SaveFn saveFn)
	{
		BX_ASSERT(handle != RESOURCE_HANDLE_INVALID, "Resource handle is invalid!");

		auto it = m_database.find(handle);
		BX_ASSERT(it != m_database.end(), "Data for handle not found!");

		if (!saveFn(filename, it->second.data))
		{
			BX_LOGE("Failed to save resource ({} | {})", handle, filename);
			return;
		}

		it->second.storage = ResourceStorage::DISK;
		it->second.filename = filename;

		BX_LOGD("Saved resource ({} | {})", handle, filename);
	}

	inline void Unload(ResourceHandle handle, UnloadFn unloadFn)
	{
		BX_ASSERT(handle != RESOURCE_HANDLE_INVALID, "Resource handle is invalid!");

		auto it = m_database.find(handle);
		if (it == m_database.end())
			return;

		unloadFn(it->second.data);

		m_database.erase(it);
		BX_LOGD("Unloaded resource ({})", handle);
	}

	inline void IncreaseRefCount(ResourceHandle handle)
	{
		BX_ENSURE(handle != RESOURCE_HANDLE_INVALID);

		auto it = m_database.find(handle);
		BX_ENSURE(it != m_database.end());

		it->second.refCount++;
		BX_LOGD("Resource ({}) increment ref count {}", handle, it->second.refCount);
	}

	inline void DecreaseRefCount(ResourceHandle handle, UnloadFn unloadFn)
	{
		BX_ENSURE(handle != RESOURCE_HANDLE_INVALID);

		auto it = m_database.find(handle);
		BX_ENSURE(it != m_database.end());

		BX_ENSURE(it->second.refCount > 0);
		it->second.refCount--;
		BX_LOGD("Resource ({}) decrement ref count {}", handle, it->second.refCount);

		if (it->second.refCount == 0)
		{
			Unload(handle, unloadFn);
		}
	}

private:
	HashMap<ResourceHandle, ResourceData<TData>> m_database;
};

template <typename TData>
class Resource
{
public:
	Resource() {}

	Resource(const String& filename)
		: m_handle(MakeHandle(filename))
	{
		if (IsValid())
		{
			if (!Load(filename))
			{
				m_handle = RESOURCE_HANDLE_INVALID;
				return;
			}

			IncreaseRefCount();
		}
	}

	Resource(ResourceHandle handle, const TData& data)
		: m_handle(handle)
	{
		if (IsValid())
		{
			LoadData(data);
			IncreaseRefCount();
		}
	}

	~Resource()
	{
		if (IsValid())
			DecreaseRefCount();
	}

	Resource(const Resource& other)
		: m_handle(other.m_handle), m_uuid(other.m_uuid)
	{
		if (IsValid())
			IncreaseRefCount();
	}

	Resource(Resource&& other) noexcept
		: m_handle(other.m_handle), m_uuid(other.m_uuid)
	{
		other.m_handle = RESOURCE_HANDLE_INVALID;
		other.m_uuid = 0;
	}

	Resource& operator=(const Resource& other)
	{
		if (m_handle != other.m_handle)
		{
			if (IsValid())
				DecreaseRefCount();

			m_handle = other.m_handle;

			if (IsValid())
				IncreaseRefCount();
		}
		return *this;
	}

	Resource& operator=(Resource&& other) noexcept
	{
	if (this != &other)
		{
			if (IsValid())
				DecreaseRefCount();

			m_handle = other.m_handle;
			other.m_handle = RESOURCE_HANDLE_INVALID;
		}
		return *this;
	}

	inline ResourceHandle GetHandle() const { return m_handle; }
	inline bool IsValid() const { return m_handle != RESOURCE_HANDLE_INVALID; }

	inline bool operator==(const Resource& other) const { return m_handle == other.m_handle; }
	//inline bool operator==(const IResource& other) const { return m_handle == other.m_handle; }

	// Utility operators for ease of use
	inline operator bool() const { return IsValid() && IsLoaded(); }
	inline const TData* operator->() const { return GetDataPtr(); }

	inline TData& GetData() const
	{
		return GetDatabase().GetData(m_handle);
	}

	inline const TData* GetDataPtr() const
	{
		return GetDatabase().GetDataPtr(m_handle);
	}

	inline const ResourceData<TData>& GetResourceData() const
	{
		return GetDatabase().GetResourceData(m_handle);
	}

	inline bool IsLoaded() const
	{
		return GetDatabase().IsLoaded(m_handle);
	}

	inline void Save(const String& filename) const
	{
		GetDatabase().Save(m_handle, filename, &Resource::Save);
	}

private:
	inline bool Load(const String& filename) const
	{
		return GetDatabase().Load(m_handle, filename, &Resource::Load);
	}

	inline void LoadData(const TData& data) const
	{
		return GetDatabase().LoadData(m_handle, data);
	}

	inline void Unload() const
	{
		GetDatabase().Unload(m_handle, &Resource::Unload);
	}

	inline void IncreaseRefCount() const
	{
		GetDatabase().IncreaseRefCount(m_handle);
	}

	inline void DecreaseRefCount() const
	{
		GetDatabase().DecreaseRefCount(m_handle, &Resource::Unload);
	}

public:
	static ResourceHandle MakeHandle(const String& filename)
	{
		static const Hash<String> s_stringHashFn;
		return Type<TData>::Id() ^ s_stringHashFn(filename);
	}

	static bool Save(const String& filename, const TData& data);
	static bool Load(const String& filename, TData& data);
	static void Unload(TData& data);

private:
	template <typename T>
	friend class Serial;

	static ResourceDatabase<TData>& GetDatabase()
	{
		static ResourceDatabase<TData> database;
		return database;
	}

private:
	ResourceHandle m_handle = RESOURCE_HANDLE_INVALID;
	UUID m_uuid;
};