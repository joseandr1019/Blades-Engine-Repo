#pragma once
#include <string>
#include <unordered_map>

#include "SDL2_Mix/SDL_mixer.h"

class AudioDB {
public:
	static inline std::unordered_map<std::string, Mix_Chunk*> sound_chunks;

	static void LuaInit();
private:
	AudioDB() {}
};