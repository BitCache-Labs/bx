#pragma once

#include <bx/core/object.hpp>

class Selection
{
public:
	ObjectRef GetSelected();
	void SetSelected(ObjectRef selected);

	void ClearSelection();

private:
	ObjectRef m_selected = ObjectRef::Invalid();
};