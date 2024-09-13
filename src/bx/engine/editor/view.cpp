#include "bx/editor/core/view.hpp"

#include <bx/engine/containers/list.hpp>

struct InspectorView
{
	bool show = false;
	ViewInitializeCallback initialize = nullptr;
	ViewShutdownCallback shutdown = nullptr;
	ViewPresentCallback present = nullptr;
};

static List<InspectorView> s_views;

void View::Initialize()
{
}

void View::Shutdown()
{
}

void View::Present()
{
	for (auto& view : s_views)
	{
		view.present(view.show);
	}
}

void View::Add(ViewInitializeCallback initialize, ViewShutdownCallback shutdown, ViewPresentCallback present)
{
	InspectorView view;
	view.initialize = initialize;
	view.shutdown = shutdown;
	view.present = present;
	s_views.emplace_back(view);
}