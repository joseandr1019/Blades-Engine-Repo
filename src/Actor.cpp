#include "Actor.h"
#include "ComponentDB.h"
#include "SceneDB.h"

#include <stdexcept>

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"

bool FindKeyFromAdds(std::vector<std::shared_ptr<luabridge::LuaRef>> components, const std::string& key) {
	for (auto& component : components) {
		if ((*component)["key"].cast<std::string>() == key) {
			return true;
		}
	}
	return false;
}

luabridge::LuaRef Actor::GetComponentByKey(const std::string& key) {
	try {
		auto& ref = *keyedComponents.at(key);
		if (!ref["removed"].cast<bool>()) {
			return ref;
		}
		else {
			return luabridge::LuaRef(ComponentDB::GetLuaState());
		}
	}
	catch (const std::out_of_range&) {
		return luabridge::LuaRef(ComponentDB::GetLuaState());
	}
}

luabridge::LuaRef Actor::GetComponent(const std::string& type_name) {
	try {
		auto& ref = *typedComponents.at(type_name).at(0);
		if (!ref["removed"].cast<bool>()) {
			return ref;
		}
		else {
			return luabridge::LuaRef(ComponentDB::GetLuaState());
		}
	}
	catch (const std::out_of_range&) {
		return luabridge::LuaRef(ComponentDB::GetLuaState());
	}
}

luabridge::LuaRef Actor::GetComponents(const std::string& type_name) {
	luabridge::LuaRef componentTable = luabridge::newTable(ComponentDB::GetLuaState());
	int iter = 1;

	try {
		auto& vec = typedComponents.at(type_name);
		for (auto& component : vec) {
			auto& ref = *component;
			if (!ref["removed"].cast<bool>()) {
				componentTable[iter] = *component;
				iter++;
			}
		}
	}
	catch (const std::out_of_range&) {
		return componentTable;
	}

	return componentTable;
}

luabridge::LuaRef Actor::AddComponent(const std::string& type_name) {
	if (this->removed) return luabridge::LuaRef(ComponentDB::GetLuaState());
	if (SceneDB::nowClearing && !this->dontDestroyOnLoad) return luabridge::LuaRef(ComponentDB::GetLuaState());
	auto ref = ComponentDB::RuntimeComponentLoad(this, type_name);
	if (std::find(SceneDB::compAddedActors.begin(), SceneDB::compAddedActors.end(), this)
		== SceneDB::compAddedActors.end()) {
		SceneDB::compAddedActors.push_back(this);
		SceneDB::ActorInsertSort(SceneDB::compAddedActors);
	}
	return *ref;
}

void Actor::RemoveComponent(luabridge::LuaRef component) {
	if (this->removed) return;
	if (SceneDB::nowClearing && !this->dontDestroyOnLoad) return;
	if (this->keyedComponents.find(component["key"].cast<std::string>()) != this->keyedComponents.end()) {
		component["enabled"] = false;
		component["removed"] = true;
		auto& ref = this->keyedComponents.at(component["key"].cast<std::string>());
		if (!SceneDB::nowTrimming) {
			if (std::find(this->removedComponents.begin(), this->removedComponents.end(), ref) == this->removedComponents.end()) {
				this->removedComponents.push_back(ref);
			}
			if (std::find(SceneDB::compRemovedActors.begin(), SceneDB::compRemovedActors.end(), this)
				== SceneDB::compRemovedActors.end()) {
				SceneDB::compRemovedActors.push_back(this);
				SceneDB::ActorInsertSort(SceneDB::compRemovedActors);
			}
		}
		else {
			if (std::find(this->willRemoveComponents.begin(), this->willRemoveComponents.end(), ref) != this->willRemoveComponents.end()) {
				this->willRemoveComponents.push_back(ref);
			}
			if (std::find(SceneDB::willCompRemoveActors.begin(), SceneDB::willCompRemoveActors.end(), this)
				== SceneDB::willCompRemoveActors.end()) {
				SceneDB::willCompRemoveActors.push_back(this);
				SceneDB::ActorInsertSort(SceneDB::willCompRemoveActors);
			}
		}
	}
	else if (FindKeyFromAdds(addedComponents, component["key"].cast<std::string>())) {
		component["enabled"] = false;
		component["removed"] = true;
	}
}

void Actor::DontSave() {
	if (this->save_type == SAVE_SCENE) {
		if (auto it = std::find(SceneDB::sceneSaveActors.begin(), SceneDB::sceneSaveActors.end(), this);
			it != SceneDB::sceneSaveActors.end()) {
			SceneDB::sceneSaveActors.erase(it);
		}
	}
	else if (this->save_type == SAVE_SYSTEM) {
		if (auto it = std::find(SceneDB::systemSaveActors.begin(), SceneDB::systemSaveActors.end(), this);
			it != SceneDB::systemSaveActors.end()) {
			SceneDB::systemSaveActors.erase(it);
		}
	}
	this->save_type = SAVE_NONE;
}

void Actor::SceneSave() {
	if (this->save_type == SAVE_SYSTEM) {
		if (auto it = std::find(SceneDB::systemSaveActors.begin(), SceneDB::systemSaveActors.end(), this);
			it != SceneDB::systemSaveActors.end()) {
			SceneDB::systemSaveActors.erase(it);
		}
	}

	if (auto it = std::find(SceneDB::sceneSaveActors.begin(), SceneDB::sceneSaveActors.end(), this);
		it == SceneDB::sceneSaveActors.end()) {
		SceneDB::sceneSaveActors.push_back(this);
	}
	this->save_type = SAVE_SCENE;
}

void Actor::SystemSave() {
	if (this->save_type == SAVE_SCENE) {
		if (auto it = std::find(SceneDB::sceneSaveActors.begin(), SceneDB::sceneSaveActors.end(), this);
			it != SceneDB::sceneSaveActors.end()) {
			SceneDB::sceneSaveActors.erase(it);
		}
	}

	if (auto it = std::find(SceneDB::systemSaveActors.begin(), SceneDB::systemSaveActors.end(), this);
		it == SceneDB::systemSaveActors.end()) {
		SceneDB::systemSaveActors.push_back(this);
	}
	this->save_type = SAVE_SYSTEM;
}

void Actor::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginClass<Actor>("Actor")
		.addFunction("GetName", &Actor::GetName)
		.addFunction("GetID", &Actor::GetID)
		.addFunction("GetComponentByKey", &Actor::GetComponentByKey)
		.addFunction("GetComponent", &Actor::GetComponent)
		.addFunction("GetComponents", &Actor::GetComponents)
		.addFunction("AddComponent", &Actor::AddComponent)
		.addFunction("RemoveComponent", &Actor::RemoveComponent)
		.addFunction("DontSave", &Actor::DontSave)
		.addFunction("SceneSave", &Actor::SceneSave)
		.addFunction("SystemSave", &Actor::SystemSave)
		.endClass();
}