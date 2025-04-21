#ifndef ENGINEUTILS_H
#define ENGINEUTILS_H

#include "Renderer.h"
#include "TextDB.h"

#include <filesystem>
#include <iostream>
#include <string>

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "SDL2_TTF/SDL_ttf.h"

static void ReadJsonFile(const std::string& path, rapidjson::Document& out_document)
{
	FILE* file_pointer = nullptr;
#ifdef _WIN32
	fopen_s(&file_pointer, path.c_str(), "rb");
#else
	file_pointer = fopen(path.c_str(), "rb");
#endif
	char buffer[65536];
	rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
	out_document.ParseStream(stream);
	std::fclose(file_pointer);

	if (out_document.HasParseError()) {
		rapidjson::ParseErrorCode errorCode = out_document.GetParseError();
		std::cout << "error parsing json at [" << path << "]" << std::endl;
		exit(0);
	}
}

static void WriteJSONFile(const std::string& write_path, rapidjson::Document& in_document) {
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	in_document.Accept(writer);

	std::ofstream temp(write_path);
	if (temp.is_open()) {
		temp << buffer.GetString();
		temp.close();
	}
}

static void LoadGame(rapidjson::Document& configJson) {
	ReadJsonFile("resources/game.config", configJson);

	if (configJson.HasMember("game_title")) {
		Renderer::GAME_TITLE = configJson["game_title"].GetString();
	}

	if (!configJson.HasMember("initial_scene")) {
		std::cout << "error: initial_scene unspecified";
		std::exit(0);
	}
}

static void LoadRendering(rapidjson::Document& configJson) {
	if (std::filesystem::exists(("resources/rendering.config"))) {

		ReadJsonFile("resources/rendering.config", configJson);

		if (configJson.HasMember("x_resolution")) {
			Renderer::SetCameraWidth(configJson["x_resolution"].GetInt());
		}

		if (configJson.HasMember("y_resolution")) {
			Renderer::SetCameraHeight(configJson["y_resolution"].GetInt());
		}

		if (configJson.HasMember("clear_color_r")) {
			Renderer::CLEAR_COLOR.r = configJson["clear_color_r"].GetInt();
		}

		if (configJson.HasMember("clear_color_g")) {
			Renderer::CLEAR_COLOR.g = configJson["clear_color_g"].GetInt();
		}

		if (configJson.HasMember("clear_color_b")) {
			Renderer::CLEAR_COLOR.b = configJson["clear_color_b"].GetInt();
		}

		if (configJson.HasMember("zoom_factor")) {
			Renderer::RENDER_SCALE = configJson["zoom_factor"].GetFloat();
		}
	}
}

#endif