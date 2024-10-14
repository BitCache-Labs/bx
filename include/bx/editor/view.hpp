#pragma once

#include <bx/bx.hpp>
#include <bx/editor/selection.hpp>
#include <bx/editor/command.hpp>

#include <rttr/rttr_enable.h>

class BX_API View
{
	RTTR_ENABLE()

public:
	virtual ~View() {}

	virtual bool Initialize() = 0;
	virtual void Reload() = 0;
	virtual void Shutdown() = 0;

	virtual void Present() = 0;

	inline bool IsOpen() const { return m_open; }
	inline void ToggleOpen() { m_open = !m_open; }

protected:
	Selection m_selection;
	bool m_open = false;
};