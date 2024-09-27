#pragma once

#include <bx/core/byte_types.hpp>
#include <bx/containers/list.hpp>

struct Command
{
	virtual void Do() = 0;
	virtual void Undo() = 0;
};

class CommandHistory
{
public:
	Command& Add(Command* pCmd);

	bool CanRedo();
	bool CanUndo();

	void Redo();
	void Undo();

private:
	List<Command*> m_commands;
	SizeType m_current = 0;
};