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
    FilePath assetPath{};
    FilePath importedPath{};
    CString<16> creationDate{};

    Array<int, 16> arr;

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

    template <typename T>
    void Load()
    {
        const auto& data = GetData();

        // Check if already loaded
        if (data.object.IsValid())
            return;

        auto stream = File::Get().InputStream(data.metadata.assetPath);
        if (!stream.is_open())
        {
            //BX_LOGE("Failed to open asset file: {}", data.metadata.assetPath);
            return;
        }

        cereal::JSONInputArchive archive(stream);
        archive(cereal::make_nvp("data", *data.object.As<T>()));

        //BX_LOGD("Loaded asset {} from {}", m_handle, metadata.assetPath);
    }

    template <typename T>
    void Save()
    {
        const auto& data = GetData();

        BX_ENSURE(data.object.IsValid());
        auto obj = data.object.As<T>();

        {
            auto stream = File::Get().OutputStream(data.metadata.assetPath);
            if (!stream.is_open())
            {
                //BX_LOGE("Failed to save asset file: {}", data.metadata.assetPath);
                return;
            }

            cereal::JSONOutputArchive archive(stream);
            archive(cereal::make_nvp("metadata", data.metadata));
            archive(cereal::make_nvp("data", *obj));
        }

        //BX_LOGD("Saved asset {} to {}", m_uuid, data.metadata.assetPath);
    }

    inline const AssetData& GetData() const
    {
        const auto& assetToData = GetAssetToDataMap();
        auto it = assetToData.find(m_uuid);
        BX_ENSURE(it != assetToData.end());
        return it->second;
    }

public:
    template <typename T>
    static Asset CreateAsset(Object<T> object, StringView filename)
    {
        AssetData data{};
        data.metadata.uuid = GenUUID::MakeUUID();
        data.metadata.assetPath = filename;
        // TODO: Rest of metadata
        data.object = object;

        auto& assetToData = GetAssetToDataMap();
        auto& objectToAsset = GetObjectToAssetMap();

        assetToData[data.metadata.uuid] = data;
        objectToAsset[data.object] = data.metadata.uuid;

        return Asset(data.metadata.uuid);
    }

    static bool IsAsset(const ObjectHandle& obj)
    {
        const auto& objectToAsset = GetObjectToAssetMap();
        return objectToAsset.find(obj) != objectToAsset.end();
    }

    static Asset GetAsset(const ObjectHandle& obj)
    {
        const auto& objectToAsset = GetObjectToAssetMap();
        auto it = objectToAsset.find(obj);
        return (it != objectToAsset.end()) ? Asset(it->second) : Asset();
    }

private:
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
