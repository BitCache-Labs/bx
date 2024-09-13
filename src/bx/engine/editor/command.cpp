#include "bx/editor/core/command.hpp"

#include <bx/engine/core/byte_types.hpp>
#include <bx/engine/containers/list.hpp>

static List<ICommand*> g_commands;
static SizeType g_current = 0;

ICommand& CommandHistory::Add(ICommand* pCmd)
{
	if (CanRedo())
	{
		for (SizeType i = g_current; i < g_commands.size(); i++)
			delete g_commands[i];

		g_commands.erase(g_commands.begin() + g_current, g_commands.begin() + (g_commands.size() - g_current));
	}

	g_commands.emplace_back(pCmd);
	g_current++;

	return *pCmd;
}

bool CommandHistory::CanRedo()
{
	return g_current < g_commands.size();
}

bool CommandHistory::CanUndo()
{
	return g_current > 0;
}

void CommandHistory::Redo()
{
	if (!CanRedo()) return;

	auto pCmd = g_commands[g_current];
	pCmd->Do();
	g_current++;
}

void CommandHistory::Undo()
{
	if (!CanUndo()) return;

	auto pCmd = g_commands[g_current - 1];
	pCmd->Undo();
	g_current--;
}