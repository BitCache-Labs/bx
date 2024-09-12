#pragma once

#include "bx/engine/core/resource.hpp"

#include "bx/engine/core/serial.serial.hpp"
#include "bx/engine/containers/string.serial.hpp"

template <typename TData>
class Serial<Resource<TData>>
{
public:
	template<class Archive>
	static void Save(Archive& ar, const Resource<TData>& res)
	{
		if (!res.IsValid())
		{
			ar(cereal::make_nvp("storage", (int)ResourceStorage::NONE));
			return;
		}

		const auto& data = res.GetResourceData();
		ar(cereal::make_nvp("storage", (int)data.storage));

		switch (data.storage)
		{
		case ResourceStorage::DISK:
			ar(cereal::make_nvp("filename", data.filename));
			break;

		case ResourceStorage::MEMORY:
		//	ar(cereal::make_nvp("handle", res.GetHandle()));
		//	ar(cereal::make_nvp("data", data.data));
			BX_ASSERT(false, "TODO: Currently not supported!");
			break;
		}
	}

	template<class Archive>
	static void Load(Archive& ar, Resource<TData>& res)
	{
		int storage;
		ar(cereal::make_nvp("storage", storage));

		if (storage == (int)ResourceStorage::NONE)
		{
			return;
		}

		String filename;

		// TODO: Unused
		//ResourceHandle handle;
		//TData data;

		switch ((ResourceStorage)storage)
		{
		case ResourceStorage::DISK:
			ar(cereal::make_nvp("filename", filename));
			res = Resource<TData>(filename);
			break;

		case ResourceStorage::MEMORY:
		//	ar(cereal::make_nvp("handle", handle));
		//	ar(cereal::make_nvp("data", data));
		//	res = ResourceManager::Create<TData>(handle, data);
			BX_ASSERT(false, "TODO: Currently not supported!");
			break;
		}
	}
};

REGISTER_SERIAL_T(Resource)