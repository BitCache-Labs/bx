#include "bx/engine/resource.hpp"
#include "bx/engine/resource.serial.hpp"

static HashMap<ResourceHandle, SizeType> g_refCountMap;

void ResourceManager::Initialize()
{
}

void ResourceManager::Shutdown()
{
	for (auto database : IResourceDatabase::GetDatabaseRecord())
		database->Shutdown();

	IResourceDatabase::GetDatabaseRecord().clear();
}

HashMap<ResourceHandle, SizeType>& ResourceManager::GetRefCountMap()
{
	return g_refCountMap;
}