#pragma once

#include <engine/api.hpp>
#include <engine/memory.hpp>
#include <engine/type.hpp>
#include <engine/uuid.hpp>
#include <engine/hash_map.hpp>
#include <engine/object.hpp>
#include <engine/file.hpp>
#include <engine/serial.hpp>

#include <engine/array.hpp>

enum struct AssetStorage { NONE, DISK, MEMORY };

struct BX_API AssetMetadata
{
    UUID uuid{ 0 };
    u32 version{ 1 };
    AssetStorage storage{ AssetStorage::NONE };

    // TODO: Find a way to register CString<N> type in RTTR
    //FilePath assetPath{};
    //FilePath importedPath{};
    String assetPath{};
    String importedPath{};
    String type{};
    //char test[16]{ "Hello" };

    BX_TYPE(AssetMetadata)
};

struct BX_API AssetData
{
    AssetMetadata metadata{};
    ObjectHandle object{};
};

class BX_API Asset
{
public:
    explicit Asset(UUID uuid = 0)
        : m_uuid(uuid)
    {}

    inline UUID GetUUID() const { return m_uuid; }
    inline bool IsValid() const { return m_uuid != 0; }
    operator bool() const { return IsValid(); }

    inline bool operator==(const Asset& other) const { return m_uuid == other.m_uuid; }
    inline bool operator<(const Asset& other) const { return m_uuid < other.m_uuid; }

    inline const AssetMetadata& GetMetadata() const { return GetData().metadata; }
    inline ObjectHandle GetObject() const { return GetData().object; }

    template <typename T>
    void Load()
    {
        auto& data = GetData();

        // Check if already loaded
        if (data.object.IsValid())
            return;

        auto stream = File::Get().InputStream(data.metadata.assetPath);
        if (!stream.is_open())
            return;

        auto obj = Object<T>::New();

        cereal::JSONInputArchive archive(stream);
        archive(cereal::make_nvp("data", *obj));

        data.object = obj;
    }

    template <typename T>
    void Save(StringView filename)
    {
        auto& data = GetData();
        BX_ENSURE(data.object.IsValid());

        AssetMetadata metadata = data.metadata;
        Object<T> object = data.object.As<T>();

        // Create a new asset copy
        if (data.metadata.storage == AssetStorage::DISK &&
            data.metadata.assetPath.compare(filename.to_string()) != 0)
        {
            object = Object<T>::New(*object);

            Asset asset = FromFile(filename);
            if (!asset.IsValid())
                m_uuid = GenUUID::MakeUUID();
            else
                m_uuid = asset.m_uuid;

            metadata.uuid = m_uuid;
            metadata.assetPath = filename.to_string();
            metadata.importedPath = "";

            auto& assetToData = GetAssetToDataMap();
            auto& objectToAsset = GetObjectToAssetMap();
            assetToData[metadata.uuid] = AssetData{ metadata, object };
            objectToAsset[object] = metadata.uuid;
        }

        // Update asset metadata storage to disk
        if (data.metadata.storage == AssetStorage::MEMORY)
        {
            metadata.storage = AssetStorage::DISK;
            metadata.assetPath = filename.to_string();

            data.metadata = metadata;
        }

        {
            auto stream = File::Get().OutputStream(metadata.assetPath);
            if (!stream.is_open())
            {
                return;
            }

            cereal::JSONOutputArchive archive(stream);
            archive(cereal::make_nvp("metadata", metadata));
            archive(cereal::make_nvp("data", *object));
        }
    }

public:
    static Asset FromFile(StringView filename)
    {
        AssetData data{};

        auto stream = File::Get().InputStream(filename);
        if (!stream.is_open())
            return Asset{};

        cereal::JSONInputArchive archive(stream);
        archive(cereal::make_nvp("metadata", data.metadata));
        data.metadata.storage = AssetStorage::DISK; // Hack for now until storage serialization works

        auto& assetToData = GetAssetToDataMap();
        if (assetToData.find(data.metadata.uuid) == assetToData.end())
            assetToData.insert(std::make_pair(data.metadata.uuid, data));
        
        return Asset(data.metadata.uuid);
    }

    template <typename T>
    static Asset FromMemory(Object<T> object)
    {
        AssetData data{};
        data.metadata.uuid = GenUUID::MakeUUID();
        data.metadata.storage = AssetStorage::MEMORY;
        data.metadata.type = Type<T>().Name();
        data.object = object;

        auto& assetToData = GetAssetToDataMap();
        auto& objectToAsset = GetObjectToAssetMap();
        assetToData[data.metadata.uuid] = data;
        objectToAsset[data.object] = data.metadata.uuid;

        return Asset(data.metadata.uuid);
    }

    static bool IsObjectAsset(const ObjectHandle& obj)
    {
        const auto& objectToAsset = GetObjectToAssetMap();
        return objectToAsset.find(obj) != objectToAsset.end();
    }

    static Asset GetObjectAsset(const ObjectHandle& obj)
    {
        const auto& objectToAsset = GetObjectToAssetMap();
        auto it = objectToAsset.find(obj);
        return (it != objectToAsset.end()) ? Asset(it->second) : Asset();
    }

private:
    inline AssetData& GetData() const
    {
        auto& assetToData = GetAssetToDataMap();
        auto it = assetToData.find(m_uuid);
        BX_ENSURE(it != assetToData.end());
        return it->second;
    }

    static HashMap<UUID, AssetData>& GetAssetToDataMap()
    {
        static HashMap<UUID, AssetData> g_assetToData{};
        return g_assetToData;
    }

    static HashMap<ObjectHandle, UUID>& GetObjectToAssetMap()
    {
        static HashMap<ObjectHandle, UUID> g_objectToAsset{};
        return g_objectToAsset;
    }

private:
    UUID m_uuid{ 0 };
};
