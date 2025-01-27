#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API SettingsEditor final
	: public EditorWindow
{
	BX_TYPE(SettingsEditor, EditorWindow)

public:
	bool Initialize() override;
	void Reload() override;
	void Shutdown() override;

	const char* GetTitle() const override;
	void Present(const char* title, bool& isOpen) override;
};