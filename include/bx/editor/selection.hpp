#pragma once

#include <bx/core/object.hpp>

class Selection
{
public:
	static ObjectRef GetSelected();
	static void SetSelected(ObjectRef selected);

	static void ClearSelection();
};