#pragma once

#include <bx/bx.hpp>
#include <bx/core/uuid.hpp>
#include <bx/containers/list.hpp>

#include <rttr/rttr_enable.h>
#include <rttr/registration_friend>

#include <memory>
#include <algorithm>

class BX_API View
{
	RTTR_ENABLE()

public:
	View();
	virtual ~View();

	virtual bool Initialize() = 0;
	virtual void Reload() = 0;
	virtual void Shutdown() = 0;

	virtual const char* GetTitle() const = 0;
	virtual void Present(const char* title, bool& isOpen) = 0;

	inline bool IsOpen() const { return m_isOpen; }
	inline void Close() { m_isOpen = false; }

private:
	friend class ViewManager;
	RTTR_REGISTRATION_FRIEND

	bool m_isOpen = true;
	UUID m_viewId = 0;
};

class BX_API ViewManager
{
public:
	static ViewManager& Get();

public:
	bool Initialize();
	void Shutdown();

	void Reload();
	void Present();

	template <typename TView>
	inline void AddView()
	{
		m_views.emplace_back(std::make_shared<TView>());
		m_views.back()->Initialize();
	}

	inline void AddView(const std::shared_ptr<View>& view)
	{
		if (std::find(m_views.begin(), m_views.end(), view) == m_views.end())
		{
			view->Initialize();
			m_views.push_back(view);
		}
	}

private:
	List<std::shared_ptr<View>> m_views;
};