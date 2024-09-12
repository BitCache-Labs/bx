#pragma once

#include "bx/engine/core/type.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/hash_map.hpp"

// TODO: This whole things needs to be redone, it was taken from Bojan's xs project
// but it needs to be reworked whilst keeping the original intent of functionality.
// Aka, a central data storage for various kinds of settings.
// Note: If the serialization library can handle any abstract type then this may take advantage of that by not having to specify 
// a hashmap for every supported type. Possibly a generic abstract table?

#ifdef BX_EDITOR_BUILD
ENUM(DataTarget,
	NONE = 0,
	SYSTEM = 1,
	GAME = 2,
	PLAYER = 3,
	DEBUG = 4,
	EDITOR = 5
	);
#else
ENUM(DataTarget,
	NONE = 0,
	SYSTEM = 1,
	GAME = 2,
	PLAYER = 3,
	DEBUG = 4
);
#endif

ENUM(DataType,
	NONE = 0,
	BOOL = 1,
	INT = 2,
	UINT = 3,
	FLOAT = 4,
	DOUBLE = 5,
	STRING = 6);

struct DataRecord
{
	String name = "";
	DataTarget target = DataTarget::NONE;
	DataType type = DataType::NONE;
	bool active = false;

	inline bool operator <(const DataRecord& other) const { return (name < other.name); }
	template <class Archive> void serialize(Archive& archive) { archive(name, target, type, active); }
};

class Database
{
public:
	template <typename T>
	HashMap<String, T>& GetStorage();
		
	template <class Archive> void serialize(Archive& archive) { archive(m_bools, m_ints, m_uints, m_floats, m_doubles, m_strings); }

private:
	HashMap<String, bool> m_bools;
	HashMap<String, int> m_ints;
	HashMap<String, unsigned> m_uints;
	HashMap<String, float> m_floats;
	HashMap<String, double> m_doubles;
	HashMap<String, String> m_strings;
};

template <>
inline HashMap<String, bool>& Database::GetStorage<bool>() { return m_bools; };

template <>
inline HashMap<String, int>& Database::GetStorage<int>() { return m_ints; };

template <>
inline HashMap<String, unsigned>& Database::GetStorage<unsigned>() { return m_uints; };

template <>
inline HashMap<String, float>& Database::GetStorage<float>() { return m_floats; };

template <>
inline HashMap<String, double>& Database::GetStorage<double>() { return m_doubles; };

template <>
inline HashMap<String, String>& Database::GetStorage<String>() { return m_strings; };

class Data
{
public:
	static void Save(DataTarget target);
	static void Load(DataTarget target);
	static const String& GetFilepath(DataTarget target);

	//static std::vector<DataRecord>& GetRecords()
	//{
	//	static std::vector<DataRecord> records;
	//	return records;
	//}

	static Database& GetDatabase(DataTarget target)
	{
		static HashMap<DataTarget, Database> databases;
		return databases[target];
	}

	template <typename T>
	static DataType GetType();

	template <typename T>
	static T& Get(const String& name, T value, DataTarget target)
	{
		auto& database = GetDatabase(target);
		auto& storage = database.GetStorage<T>();
		auto it = storage.find(name);
		if (it == storage.end())
		{
			//DataRecord record;
			//record.name = name;
			//record.target = target;
			//record.type = GetType<T>();
			//record.active = false;
			//GetRecords().emplace_back(record);

			storage.insert(std::make_pair(name, value));
			return Get<T>(name, value, target);
		}
		return it->second;
	}

	template <typename T>
	static T& Set(const String& name, T value, DataTarget target)
	{
		auto& database = GetDatabase(target);
		auto& storage = database.GetStorage<T>();
		auto it = storage.find(name);
		if (it == storage.end())
		{
			//DataRecord record;
			//record.name = name;
			//record.target = target;
			//record.type = GetType<T>();
			//record.active = false;
			//GetRecords().emplace_back(record);

			storage.insert(std::make_pair(name, value));
			return Get<T>(name, value, target);
		}

		it->second = value;
		return it->second;
	}

	// TODO: Don't use String, use StringView
	static bool& GetBool(const String& name, bool value, DataTarget target) { return Get<bool>(name, value, target); }
	static int& GetInt(const String& name, int value, DataTarget target) { return Get<int>(name, value, target); }
	static unsigned& GetUInt(const String& name, unsigned value, DataTarget target) { return Get<unsigned>(name, value, target); }
	static float& GetFloat(const String& name, float value, DataTarget target) { return Get<float>(name, value, target); }
	static double& GetDouble(const String& name, double value, DataTarget target) { return Get<double>(name, value, target); }
	static String& GetString(const String& name, String value, DataTarget target) { return Get<String>(name, value, target); }

	static bool& SetBool(const String& name, bool value, DataTarget target) { return Set<bool>(name, value, target); }
	static int& SetInt(const String& name, int value, DataTarget target) { return Set<int>(name, value, target); }
	static unsigned& SetUInt(const String& name, unsigned value, DataTarget target) { return Set<unsigned>(name, value, target); }
	static float& SetFloat(const String& name, float value, DataTarget target) { return Set<float>(name, value, target); }
	static double& SetDouble(const String& name, double value, DataTarget target) { return Set<double>(name, value, target); }
	static String& SetString(const String& name, String value, DataTarget target) { return Set<String>(name, value, target); }

private:
	friend class Runtime;

	static void Initialize();
	static void Shutdown();
};

template <typename T>
inline DataType Data::GetType() { return DataType::NONE; }

template <>
inline DataType Data::GetType<bool>() { return DataType::BOOL; }

template <>
inline DataType Data::GetType<int>() { return DataType::INT; }

template <>
inline DataType Data::GetType<unsigned>() { return DataType::UINT; }

template <>
inline DataType Data::GetType<float>() { return DataType::FLOAT; }

template <>
inline DataType Data::GetType<double>() { return DataType::DOUBLE; }

template <>
inline DataType Data::GetType<String>() { return DataType::STRING; }