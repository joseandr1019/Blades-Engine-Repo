#ifndef ENGINE_H
#define ENGINE_H

#include "Lua/lua.hpp"

class Engine
{
public:
	static bool running;

	static void GameLoop();

	static void OnStart();
	static void OnUpdate();
	static void OnLateUpdate();
private:
	Engine();
};

#endif