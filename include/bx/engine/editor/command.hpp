#pragma once

struct ICommand
{
	virtual void Do() = 0;
	virtual void Undo() = 0;
};

class CommandHistory
{
public:
	static ICommand& Add(ICommand* pCmd);

	static bool CanRedo();
	static bool CanUndo();

	static void Redo();
	static void Undo();
};