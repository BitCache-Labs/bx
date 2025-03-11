#include <engine/asset.hpp>

BX_TYPE_REGISTRATION
{

    rttr::type::get<CString<16>>();
    rttr::registration::enumeration<AssetStorage>("AssetStorage")
        (
            rttr::value("none", AssetStorage::NONE),
            rttr::value("disk", AssetStorage::DISK),
            rttr::value("memory", AssetStorage::MEMORY)
        );
    
    rttr::registration::class_<AssetMetadata>("AssetMetadata")
    .constructor()(rttr::policy::ctor::as_object)
    .property("uuid", &AssetMetadata::uuid)
    .property("version", &AssetMetadata::version)
    //.property("storage", &AssetMetadata::storage)
    //.property("arr", &AssetMetadata::arr)
    //.property("assetPath", &AssetMetadata::assetPath)
    //.property("importedPath", &AssetMetadata::importedPath)
    //.property("creationDate", &AssetMetadata::creationDate)
    ;
}