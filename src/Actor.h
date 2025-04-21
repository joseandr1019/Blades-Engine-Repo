#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"

enum SAVE_TYPE { SAVE_NONE, SAVE_SCENE, SAVE_SYSTEM };

class Actor
{
public:
	bool dontDestroyOnLoad;
	bool removed;
	SAVE_TYPE save_type;
	std::unordered_map<std::string, std::shared_ptr<luabridge::LuaRef>> keyedComponents;
	std::unordered_map<std::string, std::vector<std::shared_ptr<luabridge::LuaRef>>> typedComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> startingComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> updatingComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> lateUpdatingComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> addedComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> removedComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> willRemoveComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> collisionEnterComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> collisionExitComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> triggerEnterComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> triggerExitComponents;
	std::vector<std::shared_ptr<luabridge::LuaRef>> destroyingComponents;

	Actor() : actor_name(""), UUID(-1), dontDestroyOnLoad(false), removed(false), save_type(SAVE_NONE) {}

	Actor(const Actor& templateActor) {
		actor_name = templateActor.actor_name;
		UUID = templateActor.UUID;
		dontDestroyOnLoad = templateActor.dontDestroyOnLoad;
		save_type = templateActor.save_type;
	}

	~Actor() {
		keyedComponents.clear();
		typedComponents.clear();
		startingComponents.clear();
		updatingComponents.clear();
		lateUpdatingComponents.clear();
		addedComponents.clear();
		removedComponents.clear();
		willRemoveComponents.clear();
		collisionEnterComponents.clear();
		collisionExitComponents.clear();
		triggerEnterComponents.clear();
		triggerExitComponents.clear();
		destroyingComponents.clear();
	}

	std::string GetName() {
		return actor_name;
	}
	void SetName(const std::string& new_name) {
		actor_name = new_name;
	}
	int GetID() {
		return UUID;
	}
	void SetID(int new_UUID) {
		UUID = new_UUID;
	}

	luabridge::LuaRef GetComponentByKey(const std::string& key);
	luabridge::LuaRef GetComponent(const std::string& type_name);
	luabridge::LuaRef GetComponents(const std::string& type_name);
	void InjectConvenienceReference(std::shared_ptr<luabridge::LuaRef>& component) {
		(*component)["actor"] = this;
	}

	luabridge::LuaRef AddComponent(const std::string& type_name);
	void RemoveComponent(luabridge::LuaRef component);

	void DontSave();
	void SceneSave();
	void SystemSave();

	static void LuaInit();
private:
	int UUID;
	std::string actor_name;
};