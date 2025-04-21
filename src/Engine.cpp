#include "ComponentDB.h"
#include "DataManager.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "InputManager.h"
#include "Renderer.h"
#include "SceneDB.h"

#include "AudioHelper.h"

#include <filesystem>
#include <iostream>

#include "Lua/lua.hpp"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "SDL2/SDL.h"
#include "SDL2_TTF/SDL_ttf.h"

bool Engine::running = true;

void Engine::GameLoop()
{
	rapidjson::Document configJson;
	rapidjson::Document renderingJson;

	if (!std::filesystem::exists("resources")) {
		std::cout << "error: resources/ missing";
		std::exit(0);
	}
	else if (!std::filesystem::exists("resources/game.config")) {
		std::cout << "error: resources/game.config missing";
		std::exit(0);
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
		return;
	}

	if (AudioHelper::Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		SDL_Quit();
		return;
	}

	AudioHelper::Mix_AllocateChannels(50);

	if (TTF_Init() == -1) {
		SDL_Quit();
		return;
	}

	LoadGame(configJson);
	std::string initScene = configJson["initial_scene"].GetString();
	SceneDB::nextScene = initScene;

	LoadRendering(renderingJson);

	SDL_Window* window = Helper::SDL_CreateWindow(Renderer::GAME_TITLE.c_str(),
		100, 100, Renderer::WINDOW_RESOLUTION.x, Renderer::WINDOW_RESOLUTION.y, SDL_WINDOW_SHOWN);

	if (window == nullptr) {
		SDL_Quit();
		return;
	}

	SDL_Renderer* renderer = Helper::SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr) {
		SDL_DestroyWindow(window);
		SDL_Quit();
		return;
	}

	Renderer::renderer_ptr = renderer;

	if (IMG_Init(IMG_INIT_PNG) == -1) {
		SDL_DestroyWindow(window);
		SDL_Quit();
		return;
	}

	ComponentDB::LuaInit();

	SceneDB::LoadScene(SceneDB::nextScene);

	SDL_RenderSetScale(Renderer::renderer_ptr, Renderer::RENDER_SCALE, Renderer::RENDER_SCALE);
	SDL_SetRenderDrawColor(Renderer::renderer_ptr, Renderer::CLEAR_COLOR.r,
		Renderer::CLEAR_COLOR.g, Renderer::CLEAR_COLOR.b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(Renderer::renderer_ptr);

	while (Engine::running)
	{
		if (SceneDB::nextScene != "") {
			SceneDB::LoadScene(SceneDB::nextScene);
		}

		Engine::OnStart();
		SDL_Event nextEvent;
		while (Helper::SDL_PollEvent(&nextEvent)) {
			Input::ProcessEvent(nextEvent);
			if (nextEvent.type == SDL_QUIT) {
				Engine::running = false;
			}
		}

		Engine::OnUpdate();
		Engine::OnLateUpdate();
		SceneDB::AddComponents();
		SceneDB::RemoveComponents();

		SceneDB::AddActors();
		SceneDB::RemoveActors();

		Renderer::RenderRenderer();
	}

	std::filesystem::remove_all("saves/temp");
	std::filesystem::create_directories("saves/temp");

	rapidjson::Document doc;
	ReadJsonFile("resources/save.config", doc);

	doc["last_index_accessed"] = DataManager::last_index_accessed;
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	std::ofstream temp("resources/save.config");
	if (temp.is_open()) {
		temp << buffer.GetString();
		temp.close();
	}
}

void Engine::OnStart() {
	for (auto actor : SceneDB::startingActors) {
		for (auto& component : actor->startingComponents) {
			try {
				if ((*component)["enabled"].cast<bool>()) {
					(*component)["OnStart"](*component);
				}
			}
			catch (const luabridge::LuaException& e) {
				ComponentDB::ReportError(actor->GetName(), e);
			}
		}
		actor->startingComponents.clear();
	}
	SceneDB::startingActors.clear();
}

void Engine::OnUpdate() {
	for (auto actor : SceneDB::updatingActors) {
		for (auto& component : actor->updatingComponents) {
			try {
				if ((*component)["enabled"].cast<bool>()) {
					(*component)["OnUpdate"](*component);
				}
			}
			catch (const luabridge::LuaException& e) {
				ComponentDB::ReportError(actor->GetName(), e);
			}
		}
	}
}

void Engine::OnLateUpdate() {
	Input::LateUpdate();
	
	for (auto actor : SceneDB::lateUpdatingActors) {
		for (auto& component : actor->lateUpdatingComponents) {
			try {
				if ((*component)["enabled"].cast<bool>()) {
					(*component)["OnLateUpdate"](*component);
				}
			}
			catch (const luabridge::LuaException& e) {
				ComponentDB::ReportError(actor->GetName(), e);
			}
		}
	}

	SceneDB::EventSubs();

	SceneDB::world.Step(1.0f / 60.0f, 8, 3);
}

