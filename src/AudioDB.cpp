#include "AudioDB.h"
#include "ComponentDB.h"

#include "AudioHelper.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "SDL2_Mix/SDL_mixer.h"

void Play(int channel, const std::string& clip_name, bool does_loop) {
	Mix_Chunk* audio_chunk = nullptr;
	try {
		audio_chunk = AudioDB::sound_chunks.at(clip_name);
	}
	catch (const std::out_of_range&) {
		std::string pathWAV = "resources/audio/" + clip_name + ".wav";
		if (!std::filesystem::exists(pathWAV)) {
			std::string pathOGG = "resources/audio/" + clip_name + ".ogg";
			if (!std::filesystem::exists(pathOGG)) {
				std::cout << "failed to play audio clip " + clip_name;
			}
			else {
				audio_chunk = AudioHelper::Mix_LoadWAV(pathOGG.c_str());
			}
		}
		else {
			audio_chunk = AudioHelper::Mix_LoadWAV(pathWAV.c_str());
		}

		AudioDB::sound_chunks[clip_name] = audio_chunk;
	}

	int loops;
	if (does_loop) {
		loops = -1;
	}
	else {
		loops = 0;
	}

	AudioHelper::Mix_PlayChannel(channel, audio_chunk, loops);
}

void Halt(int channel) {
	AudioHelper::Mix_HaltChannel(channel);
}

void SetVolume(int channel, float volume) {
	int vol = volume;
	if (vol < 0) {
		vol = 0;
	}
	else if (vol > 128) {
		vol = 128;
	}

	AudioHelper::Mix_Volume(channel, vol);
}

void AudioDB::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Audio")
		.addFunction("Play", &Play)
		.addFunction("Halt", &Halt)
		.addFunction("SetVolume", &SetVolume)
		.endNamespace();
}