#include "bx/editor/selection.hpp"

ObjectRef Selection::GetSelected()
{
	return m_selected;
}

void Selection::SetSelected(ObjectRef selected)
{
	m_selected = selected;
}

void Selection::ClearSelection()
{
	m_selected = ObjectRef::Invalid();
}