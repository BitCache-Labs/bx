#pragma once

#include <engine/application.hpp>

class EditorApplication : public Application
{
public:
	virtual void OnMainMenuBar() = 0;
};