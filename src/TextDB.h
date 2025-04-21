#pragma once
#include <queue>
#include <string>

#include "SDL2_TTF/SDL_ttf.h"

struct TextStruct {
	SDL_Color color;
	int x;
	int y;
	TTF_Font* font;
	std::string content;
};

class TextDB {
public:
	static inline std::queue<TextStruct> textDrawQueue;
	static inline std::unordered_map<std::string,
		std::unordered_map<int, TTF_Font*>> textFonts;

	static void LuaInit();
private:
	TextDB() {}
};