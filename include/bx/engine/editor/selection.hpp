#pragma once

#include <bx/engine/core/object.hpp>

class Selection
{
public:
	static ObjectRef GetSelected();
	static void SetSelected(ObjectRef selected);

	static void ClearSelection();
};