#include "bx/editor/command.hpp"

#include <bx/core/byte_types.hpp>
#include <bx/containers/list.hpp>

Command& CommandHistory::Add(Command* pCmd)
{
	if (CanRedo())
	{
		for (SizeType i = m_current; i < m_commands.size(); i++)
			delete m_commands[i];

		m_commands.erase(m_commands.begin() + m_current, m_commands.begin() + (m_commands.size() - m_current));
	}

	m_commands.emplace_back(pCmd);
	m_current++;

	return *pCmd;
}

bool CommandHistory::CanRedo()
{
	return m_current < m_commands.size();
}

bool CommandHistory::CanUndo()
{
	return m_current > 0;
}

void CommandHistory::Redo()
{
	if (!CanRedo()) return;

	auto pCmd = m_commands[m_current];
	pCmd->Do();
	m_current++;
}

void CommandHistory::Undo()
{
	if (!CanUndo()) return;

	auto pCmd = m_commands[m_current - 1];
	pCmd->Undo();
	m_current--;
}