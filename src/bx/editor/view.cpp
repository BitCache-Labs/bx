#include <bx/editor/view.hpp>

#include <bx/core/byte_types.hpp>
#include <bx/core/file.hpp>
#include <bx/core/macros.hpp>
#include <bx/engine/imgui.hpp>

#include <fstream>

#include <rttr/access_levels.h>
#include <rttr/registration>
RTTR_REGISTRATION
{
	rttr::registration::class_<View>("View")

	.method("Initialize", &View::Initialize)
	.method("Reload", &View::Reload)
	.method("Shutdown", &View::Shutdown)
	.method("GetTitle", &View::GetTitle)
	.method("Present", &View::Present)

	.method("IsOpen", &View::IsOpen)
	.method("Close", &View::Close)

	.property("m_isOpen", &View::m_isOpen)//, rttr::registration::private_access)
	.property("m_viewId", &View::m_viewId);// , rttr::registration::private_access);
}

View::View()
{
	m_viewId = GenUUID::MakeUUID();
}

View::~View()
{
}

ViewManager& ViewManager::Get()
{
	static ViewManager* s_viewManager = nullptr;
	if (!s_viewManager) s_viewManager = new ViewManager();
	return *s_viewManager;
}

bool ViewManager::Initialize()
{
	//String filePath = File::GetPath("[editor]/views.dat");
	//std::ifstream file(filePath);// , std::ios::binary);
	//
	//if (!file.is_open())
	//{
	//	BX_LOGE("Failed to open file for reading: {}", filePath);
	//	return false;
	//}
	//
	////cereal::BinaryInputArchive archive(file);
	//cereal::JSONInputArchive archive(file);
	//if (!BX_TRYCATCH(archive(m_views), "Deserialization error"))
	//{
	//	return false;
	//}
	//
	//for (auto& view : m_views)
	//{
	//	if (!view->Initialize())
	//	{
	//		BX_LOGE("One or more views failed to initialize!");
	//		return false;
	//	}
	//}
	return true;
}

void ViewManager::Shutdown()
{
	String filePath = File::GetPath("[editor]/views.dat");
	std::ofstream file(filePath);// , std::ios::binary);

	if (!file.is_open())
	{
		BX_LOGE("Failed to open file for writing: {}", filePath);
		return;
	}

	//cereal::BinaryOutputArchive archive(file);
	//cereal::JSONOutputArchive archive(file);
	//archive(m_views);
	//BX_TRYCATCH(archive(m_views), "Serialization error");

	//for (auto& view : m_views)
	//{
	//	view->Shutdown();
	//}
}

void ViewManager::Reload()
{
	for (auto& view : m_views)
	{
		view->Reload();
	}
}

void ViewManager::Present()
{
	auto views = m_views;
	for (auto& view : views)
	{
		char windowTitle[128];
		const char* baseTitle = view->GetTitle();
		std::snprintf(windowTitle, sizeof(windowTitle), "%s##%llu", baseTitle, static_cast<UUID>(view->m_viewId));

		if (view->IsOpen())
		{
			view->Present(windowTitle, view->m_isOpen);
		}

		if (!view->IsOpen())
		{
			ImGui::ClearWindowSettings(windowTitle);
			view->Shutdown();

			auto it = std::find(m_views.begin(), m_views.end(), view);
			if (it != m_views.end())
			{
				m_views.erase(it);
			}
		}
	}
}