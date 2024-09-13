#include "bx/editor/core/selection.hpp"

static ObjectRef s_selected = ObjectRef::Invalid();

ObjectRef Selection::GetSelected()
{
	return s_selected;
}

void Selection::SetSelected(ObjectRef selected)
{
	s_selected = selected;
}

void Selection::ClearSelection()
{
	s_selected = ObjectRef::Invalid();
}