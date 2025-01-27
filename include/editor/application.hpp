#pragma once

#include <engine/api.hpp>
#include <engine/application.hpp>

class BX_API EditorApplication
	: public Application
{
	BX_TYPE(EditorApplication, Application)

public:
	virtual void OnMainMenuBar() = 0;
};