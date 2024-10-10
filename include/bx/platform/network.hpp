#pragma once

#ifdef NETWORK_IMPL
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
#endif