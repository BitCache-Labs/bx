#pragma once

class Network
{
public:
	// TODO

private:
	friend class Runtime;
	friend class Module;

	static bool Initialize() { return true; }
	static void Reload() {}
	static void Shutdown() {}
};