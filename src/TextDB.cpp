#include "ComponentDB.h"
#include "TextDB.h"

#include <filesystem>

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "SDL2_TTF/SDL_ttf.h"

void Draw(const std::string& str_content, float x, float y, const std::string& font_name,
	float font_size, float r, float g, float b, float a) {
	TextStruct tex;
	tex.content = str_content;
	tex.x = x;
	tex.y = y;
	tex.color.r = r;
	tex.color.g = g;
	tex.color.b = b;
	tex.color.a = a;

	try {
		tex.font = TextDB::textFonts.at(font_name).at(font_size);
	}
	catch (const std::out_of_range&) {
		std::string path = "resources/fonts/" + font_name + ".ttf";
		if (!std::filesystem::exists(path)) {
			std::cout << "error: font " << font_name << " missing";
			std::exit(0);
		}
		auto font = TTF_OpenFont(path.c_str(), font_size);
		auto& map = TextDB::textFonts[font_name];
		map.insert(std::pair(font_size, font));
		tex.font = font;
	}

	TextDB::textDrawQueue.push(tex);
}

void TextDB::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Text")
		.addFunction("Draw", &Draw)
		.endNamespace();
}