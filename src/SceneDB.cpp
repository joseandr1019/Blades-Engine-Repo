#include "Actor.h"
#include "ComponentDB.h"
#include "DataManager.h"
#include "EngineUtils.h"
#include "ParticleSystem.h"
#include "Rigidbody.h"
#include "SceneDB.h"
#include "TemplateDB.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "box2d/box2d.h"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "rapidjson/document.h"

bool SceneDB::nowRemoving = false;
bool SceneDB::nowClearing = false;
bool SceneDB::nowTrimming = false;
std::string SceneDB::currentScene = "";
std::string SceneDB::nextScene = "";
b2World SceneDB::world = b2World(b2Vec2(0.0f, 9.8f));

int SceneDB::UUID = 0;

bool actorExists(const std::string& actorName, const int actorID) {
	if (!SceneDB::namedActors[actorName].empty()) {
		for (auto actor : SceneDB::namedActors[actorName]) {
			if (actor->GetID() == actorID) {
				return true;
			}
		}
		return false;
	}
	return false;
}

luabridge::LuaRef Find(const std::string& name) {
	try {
		luabridge::LuaRef actorRef = luabridge::newTable(ComponentDB::GetLuaState());
		actorRef = &*SceneDB::namedActors.at(name).at(0);
		return actorRef;
	}
	catch (const std::out_of_range&) {
		return luabridge::LuaRef(ComponentDB::GetLuaState());
	}
}

luabridge::LuaRef FindAll(const std::string& name) {
	luabridge::LuaRef actorTable = luabridge::newTable(ComponentDB::GetLuaState());
	int iter = 1;

	try {
		auto& vec = SceneDB::namedActors.at(name);
		for (auto actor : vec) {
			actorTable[iter] = &*actor;
			iter++;
		}
	}
	catch (const std::out_of_range&) {
		return actorTable;
	}

	return actorTable;
}

luabridge::LuaRef Instantiate(const std::string& actor_template_name) {
	if (SceneDB::nowClearing) {
		if (!TemplateDB::DoesntDestroyOnLoad(actor_template_name)) {
			return luabridge::LuaRef(ComponentDB::GetLuaState());
		}
		else {
			Actor* tempActor = new Actor();
			TemplateDB::LoadTemplate(*tempActor, actor_template_name);
			(*tempActor).SetID(SceneDB::UUID);
			SceneDB::UUID++;

			SceneDB::retainedActors.push_back(tempActor);
			SceneDB::ActorInsertSort(SceneDB::retainedActors);

			luabridge::LuaRef actorRef = luabridge::newTable(ComponentDB::GetLuaState());
			actorRef = &*tempActor;
			return actorRef;
		}
	}

	Actor* tempActor = new Actor();
	TemplateDB::LoadTemplate(*tempActor, actor_template_name);
	(*tempActor).SetID(SceneDB::UUID);
	SceneDB::UUID++;

	SceneDB::actors.push_back(tempActor);
	SceneDB::namedActors[(*tempActor).GetName()].push_back(tempActor);
	SceneDB::addedActors.push_back(tempActor);

	luabridge::LuaRef actorRef = luabridge::newTable(ComponentDB::GetLuaState());
	actorRef = &*tempActor;
	return actorRef;
}

Actor* RefToPointer(std::vector<Actor*> actors, Actor& target) {
	for (auto actor : actors) {
		if ((*actor).GetID() == target.GetID()) {
			return actor;
		}
	}
	return nullptr;
}

void Destroy(Actor* actor) {
	if (actor == nullptr) return;
	if (actor->removed) return;
	if (SceneDB::nowClearing && !actor->dontDestroyOnLoad) return;
	actor->removed = true;
	if (auto it = std::find(SceneDB::addedActors.begin(), SceneDB::addedActors.end(), actor);
		it != SceneDB::addedActors.end()) {
		SceneDB::addedActors.erase(it);
	}

	for (auto& component : actor->keyedComponents) {
		(*component.second)["enabled"] = false;
	}

	
	{
		if (std::find(SceneDB::namedActors[actor->GetName()].begin(),
			SceneDB::namedActors[actor->GetName()].end(), actor)
			!= SceneDB::namedActors[actor->GetName()].end()) {
			SceneDB::namedActors[actor->GetName()].erase(
				std::find(SceneDB::namedActors[actor->GetName()].begin(),
				SceneDB::namedActors[actor->GetName()].end(), actor));
		}
	}

	if (!SceneDB::nowRemoving) {
		if (std::find(SceneDB::removedActors.begin(),
			SceneDB::removedActors.end(), actor)
			== SceneDB::removedActors.end()) {
			SceneDB::removedActors.push_back(actor);
		}
	}
	else {
		auto& vec = SceneDB::willRemoveActors;
		if (std::find(vec.begin(), vec.end(), actor) == vec.end()) {
			SceneDB::willRemoveActors.push_back(actor);
		}
	}
}

void Load(const std::string& scene_name) {
	SceneDB::nextScene = scene_name;
}

std::string GetCurrent() {
	return SceneDB::currentScene;
}

void DontDestroy(Actor* actor) {
	actor->dontDestroyOnLoad = true;
}

void Subscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
	if (std::find(SceneDB::pending_subscriptions[event_type].begin(), SceneDB::pending_subscriptions[event_type].end(),
		std::pair(component, function)) == SceneDB::pending_subscriptions[event_type].end()) {
		SceneDB::pending_subscriptions[event_type].push_back(std::pair(component, function));
	}
}

void Unsubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
	if (std::find(SceneDB::pending_unsubscriptions[event_type].begin(), SceneDB::pending_unsubscriptions[event_type].end(),
		std::pair(component, function)) == SceneDB::pending_unsubscriptions[event_type].end()) {
		SceneDB::pending_unsubscriptions[event_type].push_back(std::pair(component, function));
	}
}

void Publish(const std::string& event_type, luabridge::LuaRef event_object) {
	for (auto& sub : SceneDB::event_bus[event_type]) {
		(sub.second)(sub.first, event_object);
	}
}

void SceneDB::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Actor")
		.addFunction("Find", &Find)
		.addFunction("FindAll", &FindAll)
		.addFunction("Instantiate", &Instantiate)
		.addFunction("Destroy", &Destroy)
		.endNamespace();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Scene")
		.addFunction("Load", &Load)
		.addFunction("GetCurrent", &GetCurrent)
		.addFunction("DontDestroy", &DontDestroy)
		.endNamespace();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Event")
		.addFunction("Publish", &Publish)
		.addFunction("Subscribe", &Subscribe)
		.addFunction("Unsubscribe", &Unsubscribe)
		.endNamespace();
}

bool IsInherited(Actor* actor, std::string key,
	rapidjson::GenericMemberIterator<false, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> value) {
	try {
		auto& component = actor->keyedComponents.at(key);
		for (auto itr = value->value.MemberBegin();
			itr != value->value.MemberEnd(); ++itr) {
			if (std::string(itr->name.GetString()) == "type") {
				continue;
			}
			else {
				ComponentDB::LoadOverride(*component, itr);
			}
		}
		return true;
	}
	catch (const std::out_of_range&) {
		return false;
	}
}

void SceneDB::LoadScene(std::string& sceneName) {
	if (sceneName == "system") {
		std::cout << "error: scene name \"system\" is reserved by engine. rename scene.";
		std::exit(0);
	}

	std::string scenePath = "resources/scenes/" + sceneName + ".scene";
	if (!std::filesystem::exists(scenePath)) {
		std::cout << "error: scene " + sceneName + " is missing";
		std::exit(0); 
	}

	SceneDB::startingActors.clear();
	SceneDB::updatingActors.clear();
	SceneDB::lateUpdatingActors.clear();
	SceneDB::namedActors.clear();
	SceneDB::compAddedActors.clear();
	SceneDB::compRemovedActors.clear();

	if (!DataManager::loadingSave) {
		DataManager::SaveScene();
	}
	SceneDB::sceneSaveActors.clear();

	SceneDB::nowClearing = true;
	for (auto actor : SceneDB::actors) {
		if (!actor->dontDestroyOnLoad || DataManager::loadingSave) {
			for (auto& component : actor->destroyingComponents) {
				try {
					(*component)["OnDestroy"](*component);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(actor->GetName(), e);
				}
			}

			if (!actor->typedComponents["Rigidbody"].empty()) {
				for (auto& component : actor->typedComponents["Rigidbody"]) {
					auto bd = (*component).cast<Rigidbody*>();
					delete bd;
				}
			}

			if (!actor->typedComponents["ParticleSystem"].empty()) {
				for (auto& component : actor->typedComponents["ParticleSystem"]) {
					auto bd = (*component).cast<ParticleSystem*>();
					delete bd;
				}
			}

			auto& vec = SceneDB::willRemoveActors;
			if (auto it = std::find(vec.begin(), vec.end(), actor); it != vec.end()) {
				vec.erase(it);
			}

			vec = SceneDB::addedActors;
			if (auto it = std::find(vec.begin(), vec.end(), actor); it != vec.end()) {
				vec.erase(it);
			}

			vec = SceneDB::removedActors;
			if (auto it = std::find(vec.begin(), vec.end(), actor); it != vec.end()) {
				vec.erase(it);
			}

			delete actor;
		}
		else {
			SceneDB::retainedActors.push_back(actor);
			SceneDB::ActorInsertSort(SceneDB::retainedActors);
		}
	}
	SceneDB::nowClearing = false;

	SceneDB::actors.clear();
	if (DataManager::loadingSave) {
		SceneDB::retainedActors.clear();
	}
	SceneDB::systemSaveActors.clear();
	for (auto actor : SceneDB::retainedActors) {
		SceneDB::namedActors[actor->GetName()].push_back(actor);

		if (!(*actor).startingComponents.empty()) {
			SceneDB::startingActors.push_back(actor);
		}
		if (!(*actor).updatingComponents.empty()) {
			SceneDB::updatingActors.push_back(actor);
		}
		if (!(*actor).lateUpdatingComponents.empty()) {
			SceneDB::lateUpdatingActors.push_back(actor);
		}
		if (!(*actor).addedComponents.empty()) {
			SceneDB::compAddedActors.push_back(actor);
		}
		if (!(*actor).removedComponents.empty()) {
			SceneDB::compRemovedActors.push_back(actor);
		}

		if (actor->save_type == SAVE_SYSTEM) {
			SceneDB::systemSaveActors.push_back(actor);
		}
	}
	for (auto actor : SceneDB::addedActors) {
		SceneDB::namedActors[actor->GetName()].push_back(actor);
		SceneDB::ActorInsertSort(SceneDB::namedActors[actor->GetName()]);

		if (!(*actor).startingComponents.empty()) {
			SceneDB::startingActors.push_back(actor);
			SceneDB::ActorInsertSort(SceneDB::startingActors);
		}
		if (!(*actor).updatingComponents.empty()) {
			SceneDB::updatingActors.push_back(actor);
			SceneDB::ActorInsertSort(SceneDB::updatingActors);
		}
		if (!(*actor).lateUpdatingComponents.empty()) {
			SceneDB::lateUpdatingActors.push_back(actor);
			SceneDB::ActorInsertSort(SceneDB::lateUpdatingActors);
		}
		if (!(*actor).addedComponents.empty()) {
			SceneDB::compAddedActors.push_back(actor);
			SceneDB::ActorInsertSort(SceneDB::compAddedActors);
		}
		if (!(*actor).removedComponents.empty()) {
			SceneDB::compRemovedActors.push_back(actor);
			SceneDB::ActorInsertSort(SceneDB::compRemovedActors);
		}
	}
	
	DataManager::loadingSave = false;
	SceneDB::currentScene = sceneName;
	SceneDB::nextScene = "";
	SceneDB::UUID = 0;
	rapidjson::Document sceneJson;
	ReadJsonFile(scenePath, sceneJson);

	rapidjson::GenericArray sceneActors = sceneJson["actors"].GetArray();

	for (auto& actor : sceneActors) {
		Actor* tempActor = new Actor();

		if (actor.HasMember("template")) {
			TemplateDB::LoadTemplate(*tempActor, std::string(actor["template"].GetString()));
		}

		if (actor.HasMember("name")) {
			(*tempActor).SetName(std::string(actor["name"].GetString()));
		}

		(*tempActor).SetID(SceneDB::UUID);
		SceneDB::UUID++;

		if (actorExists(tempActor->GetName(), tempActor->GetID())) {
			delete tempActor;
			continue;
		}

		if (actor.HasMember("components")) {
			for (auto itr = actor["components"].MemberBegin();
				itr != actor["components"].MemberEnd(); ++itr) {
				if (IsInherited(tempActor, itr->name.GetString(), itr)) {

				}
				else if (itr->value.HasMember("type")) {
					ComponentDB::LoadComponent(tempActor,
						itr->value["type"].GetString(),
						itr->name.GetString(), itr);
				}
				else {
					continue;
				}

				
			}
		}

		SceneDB::actors.push_back(tempActor);
		SceneDB::namedActors[(*tempActor).GetName()].push_back(tempActor);
		if (!(*tempActor).startingComponents.empty()) {
			SceneDB::startingActors.push_back(tempActor);
		}
		if (!(*tempActor).updatingComponents.empty()) {
			SceneDB::updatingActors.push_back(tempActor);
		}
		if (!(*tempActor).lateUpdatingComponents.empty()) {
			SceneDB::lateUpdatingActors.push_back(tempActor);
		}
	}

	DataManager::LoadScene();
	DataManager::LoadSystem();
}

void SceneDB::ActorInsertSort(std::vector<Actor*>& actors) {
	for (size_t iter = actors.size() - 1; iter > 0; iter--) {
		if (actors[iter]->GetID() < actors[iter - 1]->GetID()) {
			Actor* temp = actors[iter];
			actors[iter] = actors[iter - 1];
			actors[iter - 1] = temp;
		}
		else {
			break;
		}
	}
}

void SceneDB::AddComponents() {
	for (auto actor : SceneDB::compAddedActors) {
		for (auto& component : actor->addedComponents) {
			if (!(*component)["removed"].cast<bool>()) {
				actor->keyedComponents.insert(
					std::pair((*component)["key"].cast<std::string>(), component));
				actor->typedComponents[(*component)["type"].cast<std::string>()].push_back(component);

				if ((*component)["OnStart"].isFunction()) {
					actor->startingComponents.push_back(component);
					ComponentDB::ComponentInsertSort(actor->startingComponents);
				}

				if ((*component)["OnUpdate"].isFunction()) {
					actor->updatingComponents.push_back(component);
					ComponentDB::ComponentInsertSort(actor->updatingComponents);
				}

				if ((*component)["OnLateUpdate"].isFunction()) {
					actor->lateUpdatingComponents.push_back(component);
					ComponentDB::ComponentInsertSort(actor->lateUpdatingComponents);
				}

				if ((*component)["OnDestroy"].isFunction()) {
					actor->destroyingComponents.push_back(component);
					ComponentDB::ComponentInsertSort(actor->destroyingComponents);
				}

				if ((*component)["OnCollisionEnter"].isFunction()) {
					actor->collisionEnterComponents.push_back(component);
					ComponentDB::ComponentInsertSort(actor->collisionEnterComponents);
				}

				if ((*component)["OnCollisionExit"].isFunction()) {
					actor->collisionExitComponents.push_back(component);
					ComponentDB::ComponentInsertSort(actor->collisionExitComponents);
				}

				if ((*component)["OnTriggerEnter"].isFunction()) {
					actor->triggerEnterComponents.push_back(component);
					ComponentDB::ComponentInsertSort(actor->triggerEnterComponents);
				}

				if ((*component)["OnTriggerExit"].isFunction()) {
					actor->triggerExitComponents.push_back(component);
					ComponentDB::ComponentInsertSort(actor->triggerExitComponents);
				}
			}
		}

		actor->addedComponents.clear();
		auto& vec = SceneDB::addedActors;
		if (std::find(vec.begin(), vec.end(), actor) == vec.end()) {
			if (!actor->startingComponents.empty()) {
				SceneDB::startingActors.push_back(actor);
				SceneDB::ActorInsertSort(SceneDB::startingActors);
			}

			if (!actor->updatingComponents.empty()) {
				if (std::find(SceneDB::updatingActors.begin(),
					SceneDB::updatingActors.end(), actor) == SceneDB::updatingActors.end()) {
					SceneDB::updatingActors.push_back(actor);
					SceneDB::ActorInsertSort(SceneDB::updatingActors);
				}
			}

			if (!actor->lateUpdatingComponents.empty()) {
				if (std::find(SceneDB::lateUpdatingActors.begin(),
					SceneDB::lateUpdatingActors.end(), actor) == SceneDB::lateUpdatingActors.end()) {
					SceneDB::lateUpdatingActors.push_back(actor);
					SceneDB::ActorInsertSort(SceneDB::lateUpdatingActors);
				}
			}
		}
	}
	SceneDB::compAddedActors.clear();
}

void SceneDB::RemoveComponents() {
	SceneDB::nowTrimming = true;
	for (auto actor : SceneDB::compRemovedActors) {
		for (auto& component : actor->removedComponents) {
			std::string key = (*component)["key"].cast<std::string>();
			actor->keyedComponents.erase(key);
			auto& vec = actor->typedComponents.at((*component)["type"].cast<std::string>());
			vec.erase(std::find(vec.begin(), vec.end(), component));

			{
				auto& vec = actor->startingComponents;
				if (auto it = std::find(vec.begin(), vec.end(), component); it != vec.end()) {
					vec.erase(it);
				}
			}

			{
				auto& vec = actor->updatingComponents;
				if (auto it = std::find(vec.begin(), vec.end(), component); it != vec.end()) {
					vec.erase(it);
				}
			}

			{
				auto& vec = actor->lateUpdatingComponents;
				if (auto it = std::find(vec.begin(), vec.end(), component); it != vec.end()) {
					vec.erase(it);
				}
			}

			{
				auto& vec = actor->collisionEnterComponents;
				if (auto it = std::find(vec.begin(), vec.end(), component); it != vec.end()) {
					vec.erase(it);
				}
			}

			{
				auto& vec = actor->collisionExitComponents;
				if (auto it = std::find(vec.begin(), vec.end(), component); it != vec.end()) {
					vec.erase(it);
				}
			}

			{
				auto& vec = actor->triggerEnterComponents;
				if (auto it = std::find(vec.begin(), vec.end(), component); it != vec.end()) {
					vec.erase(it);
				}
			}

			{
				auto& vec = actor->triggerExitComponents;
				if (auto it = std::find(vec.begin(), vec.end(), component); it != vec.end()) {
					vec.erase(it);
				}
			}

			if ((*component)["OnDestroy"].isFunction()) {
				try {
					(*component)["OnDestroy"](*component);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(actor->GetName(), e);
				}
			}

			if ((*component).isUserdata()) {
				if ((*component)["type"].cast<std::string>() != "ParticleSystem") {
					auto bd = (*component).cast<Rigidbody*>();
					delete bd;
				}
				else {
					auto bd = (*component).cast<ParticleSystem*>();
					delete bd;
				}
			}
		}
		actor->removedComponents.clear();
	}
	SceneDB::nowTrimming = false;
	SceneDB::compRemovedActors = SceneDB::willCompRemoveActors;
	SceneDB::willCompRemoveActors.clear();
}

void SceneDB::AddActors() {
	for (auto actor : SceneDB::addedActors) {
		if (!(*actor).startingComponents.empty()) {
			SceneDB::startingActors.push_back(actor);
			SceneDB::ActorInsertSort(startingActors);
		}
		if (!(*actor).updatingComponents.empty()) {
			SceneDB::updatingActors.push_back(actor);
			SceneDB::ActorInsertSort(updatingActors);
		}

		if (!(*actor).lateUpdatingComponents.empty()) {
			SceneDB::lateUpdatingActors.push_back(actor);
			SceneDB::ActorInsertSort(lateUpdatingActors);
		}
	}
	SceneDB::addedActors.clear();
}

void SceneDB::RemoveActors() {
	SceneDB::nowRemoving = true;
	for (auto actor : SceneDB::removedActors) {
		auto& vec = SceneDB::actors;
		{
			auto it = std::find(vec.begin(), vec.end(), actor);
			if (it != vec.end()) {
				vec.erase(std::find(vec.begin(), vec.end(), actor));
			}
		}

		vec = SceneDB::updatingActors;
		{
			auto it = std::find(vec.begin(), vec.end(), actor);
			if (it != vec.end()) {
				vec.erase(std::find(vec.begin(), vec.end(), actor));
			}
		}

		vec = SceneDB::lateUpdatingActors;
		{
			auto it = std::find(vec.begin(), vec.end(), actor);
			if (it != vec.end()) {
				vec.erase(std::find(vec.begin(), vec.end(), actor));
			}
		}

		vec = SceneDB::retainedActors;
		{
			auto it = std::find(vec.begin(), vec.end(), actor);
			if (it != vec.end()) {
				vec.erase(std::find(vec.begin(), vec.end(), actor));
			}
		}

		for (auto& component : actor->destroyingComponents) {
			try {
				(*component)["OnDestroy"](*component);
			}
			catch (const luabridge::LuaException& e) {
				ComponentDB::ReportError(actor->GetName(), e);
			}
		}

		actor->destroyingComponents.clear();

		vec = SceneDB::compAddedActors;
		{
			auto it = std::find(vec.begin(), vec.end(), actor);
			if (it != vec.end()) {
				vec.erase(std::find(vec.begin(), vec.end(), actor));
			}
		}

		if (!actor->typedComponents["Rigidbody"].empty()) {
			for (auto& component : actor->typedComponents["Rigidbody"]) {
				auto bd = (*component).cast<Rigidbody*>();
				delete bd;
			}
		}

		if (!actor->typedComponents["ParticleSystem"].empty()) {
			for (auto& component : actor->typedComponents["ParticleSystem"]) {
				auto bd = (*component).cast<ParticleSystem*>();
				delete bd;
			}
		}

		delete actor;
	}
	SceneDB::removedActors = SceneDB::willRemoveActors;
	SceneDB::willRemoveActors.clear();
	SceneDB::nowRemoving = false;
}

void SceneDB::EventSubs() {
	for (auto& pending_subs : SceneDB::pending_subscriptions) {
		for (auto& event_sub : pending_subs.second) {
			if (std::find(SceneDB::event_bus[pending_subs.first].begin(),
				SceneDB::event_bus[pending_subs.first].end(), event_sub)
				== SceneDB::event_bus[pending_subs.first].end()) {
				SceneDB::event_bus[pending_subs.first].push_back(event_sub);
			}
		}
	}

	for (auto& pending_unsubs : SceneDB::pending_unsubscriptions) {
		for (auto& event_sub : pending_unsubs.second) {
			auto it = std::find(SceneDB::event_bus[pending_unsubs.first].begin(),
				SceneDB::event_bus[pending_unsubs.first].end(), event_sub);
			if (it != SceneDB::event_bus[pending_unsubs.first].end()) {
				SceneDB::event_bus[pending_unsubs.first].erase(it);
			}
		}
	}
}