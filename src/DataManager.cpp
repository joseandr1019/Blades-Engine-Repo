#include "Actor.h"
#include "ComponentDB.h"
#include "DataManager.h"
#include "EngineUtils.h"
#include "SceneDB.h"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Lua/lua.h"
#include "LuaBridge/LuaBridge.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

int DataManager::num_save_index = 3;
int DataManager::last_index_accessed = 0;
bool DataManager::loadingSave = false;

void DataManager::LoadConfig() {
	// Load save.config to DataManager if it exists
	if (std::filesystem::exists("resources/save.config")) {
		rapidjson::Document doc;
		ReadJsonFile("resources/save.config", doc);
		DataManager::num_save_index = doc["num_save_index"].GetInt();
		DataManager::last_index_accessed = doc["last_index_accessed"].GetInt();
	}

	// Create save.config using defaults if it doesn't
	else {
		rapidjson::Document doc;
		doc.SetObject();
		auto& alloc = doc.GetAllocator();
		doc.AddMember("num_save_index", 3, alloc); // 3 saves are a default
		doc.AddMember("last_index_accessed", 0, alloc); // index 0 is a default

		WriteJSONFile("resources/save.config", doc);
	}

	std::string loadPath = "saves/" + std::to_string(DataManager::last_index_accessed) + "/system.save";
	if (std::filesystem::exists(loadPath)) {
		rapidjson::Document doc;
		ReadJsonFile(loadPath, doc);

		SceneDB::nextScene = doc["last_scene"].GetString();
	}
}

bool isArray(const luabridge::LuaRef& table) {
	if (table.isTable()) {
		int index = 1;
		for (luabridge::Iterator it(table); !it.isNil(); ++it) {
			if (!it.key().isNumber() || it.key().cast<int>() != index++) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool isContainer(const luabridge::LuaRef& table) {
	if (table.isTable()) {
		if (table["getType"].isFunction()) {
			return true;
		}
	}
	return false;
}

std::string serializeTable(const luabridge::LuaRef& table) {
	std::string serialize = "{";

	bool trailing = false;
	for (luabridge::Iterator it(table); !it.isNil(); ++it) {
		std::string key;
		if (it.key().isString()) {
			key = "\"" + it.key().tostring() + "\"";
		}
		else if (it.key().isNumber()) {
			key = std::to_string(it.key().cast<int>());
		}
		else {
			continue;
		}

		if (key == "\"key\"" || key == "\"removed\"" || key == "\"actor\"") {
			continue;
		}

		const luabridge::LuaRef& value = it.value();
		std::string value_str;
		if (value.isString()) {
			value_str = "\"" + value.tostring() + "\"";
		}
		else if (value.isNumber()) {
			value_str = std::to_string(value.cast<float>());
		}
		else if (value.isBool()) {
			value_str = value.cast<bool>() ? "true" : "false";
		}
		else if (isArray(value)) {
			value_str = serializeTable(value);
		}
		else if (isContainer(value)) {
			value_str = serializeTable(value);
		}
		else {
			continue;
		}

		if (trailing) serialize += ',';
		serialize += "[" + key + "]=" + value_str;
		trailing = true;
	}

	serialize += "}";
	return serialize;
}

luabridge::LuaRef deserializeTable(std::string& serialTable) {
	std::string luaCode = "return " + serialTable;
	if (luaL_dostring(ComponentDB::GetLuaState(), luaCode.c_str()) != LUA_OK) {
		std::cerr << "Failed to deserialize: " << lua_tostring(ComponentDB::GetLuaState(), -1) << std::endl;
		return luabridge::LuaRef(ComponentDB::GetLuaState());
	}

	return luabridge::LuaRef::fromStack(ComponentDB::GetLuaState(), -1);
}

void SetComponentSave(Actor* actor, rapidjson::Value& component_save, rapidjson::MemoryPoolAllocator<>& alloc) {
	for (auto& kval : actor->keyedComponents) {
		auto& comp = kval.second;
		rapidjson::Value key(kval.first.c_str(), alloc);
		rapidjson::Value value;
		value.SetString(serializeTable(*comp).c_str(), alloc);
		component_save.AddMember(key, value, alloc);
	}
}

void SaveToIndex(int index) {
	if (index < 1 || index > DataManager::num_save_index) {
		std::cout << "error: saving to an invalid index" << std::endl;
		return;
	}
	else {
		std::cout << "begin saving " << SceneDB::currentScene << "..." << std::endl;
		DataManager::SaveScene();
		DataManager::SaveSystem();
		std::cout << "begin writing " << SceneDB::currentScene << "..." << std::endl;
		DataManager::DumpTemp(index);
		std::cout << SceneDB::currentScene << " saved!" << std::endl;
	}
}

void LoadIndex(int index) {
	// Valid indexes are between 1 and the config max
	// Note: If the config max is <= 0, saves are prohibited
	if (index < 1 || index > DataManager::num_save_index) {
		std::cout << "error: loading an invalid index" << std::endl;
		return;
	}

	else {
		DataManager::last_index_accessed = index;
		std::string loadPath = "saves/" + std::to_string(DataManager::last_index_accessed) + "/system.save";

		// Loads the last scene the save was at when save occured
		if (std::filesystem::exists(loadPath)) {
			rapidjson::Document doc;
			ReadJsonFile(loadPath, doc);

			SceneDB::nextScene = doc["last_scene"].GetString();
		}

		// Loads initial scene if a save does not exist
		else {
			loadPath = "resources/game.config";
			rapidjson::Document doc;
			ReadJsonFile(loadPath, doc);

			SceneDB::nextScene = doc["initial_scene"].GetString();
		}

		// Remove Temp, and note we're loading a save
		DataManager::loadingSave = true;
		std::filesystem::remove_all("saves/temp");
		std::filesystem::create_directories("saves/temp");
	}
}

void DataManager::LuaInit() {
	DataManager::LoadConfig();
	DataManager::InitSaves();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Save")
		.addFunction("SaveToIndex", &SaveToIndex)
		.addFunction("LoadIndex", &LoadIndex)
		.endNamespace();
}

void DataManager::InitSaves() {
	if (!std::filesystem::exists("saves")) {
		std::filesystem::create_directories("saves");
	}

	if (!std::filesystem::exists("saves/temp")) {
		std::filesystem::create_directories("saves/temp");
	}

	for (int i = 1; i <= DataManager::num_save_index; i++) {
		std::string path = "saves/" + std::to_string(i);
		if (!std::filesystem::exists(path)) {
			std::filesystem::create_directories(path);
		}
	}
}

void DataManager::QueueSceneSave(Actor* actor, rapidjson::Document& doc) {
	if (!doc.IsObject()) {
		doc.SetObject();
	}

	auto& alloc = doc.GetAllocator();
	rapidjson::Value actor_save(rapidjson::kObjectType);
	rapidjson::Value component_save(rapidjson::kObjectType);
	if (doc.HasMember(actor->GetName().c_str())) {
		actor_save = doc[actor->GetName().c_str()].GetObject();
		std::string id = std::to_string(actor->GetID());
		component_save.SetObject();
		SetComponentSave(actor, component_save, alloc);
		if (actor_save.HasMember(id.c_str())) {
			actor_save[id.c_str()] = component_save;
		}
		else {
			rapidjson::Value key(id.c_str(), alloc);
			actor_save.AddMember(key, component_save, alloc);
		}
		doc[actor->GetName().c_str()] = actor_save;
	}
	else {
		actor_save.SetObject();
		std::string id = std::to_string(actor->GetID());
		component_save.SetObject();
		SetComponentSave(actor, component_save, alloc);
		rapidjson::Value key(id.c_str(), alloc);
		actor_save.AddMember(key, component_save, alloc);
		rapidjson::Value name(actor->GetName().c_str(), alloc);
		doc.AddMember(name, actor_save, alloc);
	}
}

void DataManager::QueueSystemSave(Actor* actor, rapidjson::Document& doc) {
	if (!doc.IsObject()) {
		doc.SetObject();
	}

	auto& alloc = doc.GetAllocator();
	rapidjson::Value actor_save(rapidjson::kObjectType);
	rapidjson::Value component_save(rapidjson::kObjectType);
	if (doc.HasMember(actor->GetName().c_str())) {
		actor_save = doc[actor->GetName().c_str()].GetObject();
		std::string id = std::to_string(actor->GetID());
		component_save.SetObject();
		SetComponentSave(actor, component_save, alloc);
		if (actor_save.HasMember(id.c_str())) {
			actor_save[id.c_str()] = component_save;
		}
		else {
			rapidjson::Value key(id.c_str(), alloc);
			actor_save.AddMember(key, component_save, alloc);
		}
		doc[actor->GetName().c_str()] = actor_save;
	}
	else {
		actor_save.SetObject();
		std::string id = std::to_string(actor->GetID());
		component_save.SetObject();
		SetComponentSave(actor, component_save, alloc);
		rapidjson::Value key(id.c_str(), alloc);
		actor_save.AddMember(key, component_save, alloc);
		rapidjson::Value name(actor->GetName().c_str(), alloc);
		doc.AddMember(name, actor_save, alloc);
	}
}

void DataManager::TempSceneSave(rapidjson::Document& doc) {
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	std::ofstream temp("saves/temp/" + SceneDB::currentScene + ".save");
	if (temp.is_open()) {
		temp << buffer.GetString();
		temp.close();
	}
}

void DataManager::TempSystemSave(rapidjson::Document& doc) {
	if (!doc.IsObject()) {
		doc.SetObject();
	}
	auto& alloc = doc.GetAllocator();
	if (doc.HasMember("last_scene")) {
		doc["last_scene"].SetString(SceneDB::currentScene.c_str(), alloc);
	}
	else {
		rapidjson::Value sceneValue;
		sceneValue.SetString(SceneDB::currentScene.c_str(), alloc);

		rapidjson::Value key("last_scene", alloc);
		doc.AddMember(key, sceneValue, alloc);
	}

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	std::ofstream temp("saves/temp/system.save");
	if (temp.is_open()) {
		temp << buffer.GetString();
		temp.close();
	}
}

void DataManager::SaveScene() {
	if (!SceneDB::sceneSaveActors.empty()) {
		std::string savePath = "saves/temp/" + SceneDB::currentScene + ".save";

		rapidjson::Document doc;
		if (!std::filesystem::exists(savePath)) {
			std::ofstream file(savePath, std::ios::out);
		}
		else {
			ReadJsonFile(savePath, doc);
		}
		for (auto actor : SceneDB::sceneSaveActors) {
			if (!actor->removed && !actor->dontDestroyOnLoad) {
				QueueSceneSave(actor, doc);
			}
			else if (!actor->removed && actor->dontDestroyOnLoad) {
				std::cout << "Warning: Actor " << actor->GetID() << " (" << actor->GetName()
					<< ") marked Don't Destroy on Load and Scene Save. Switch to System Save for proper behavior. Actor save voided." << std::endl;
			}
		}
		TempSceneSave(doc);
	}
}

void DataManager::SaveSystem() {
	if (!SceneDB::systemSaveActors.empty()) {
		std::string savePath = "saves/temp/system.save";

		// Creates system.save if it doesn't exist
		rapidjson::Document doc;
		if (!std::filesystem::exists(savePath)) {
			std::ofstream file(savePath, std::ios::out);
		}
		// Loads system.save if it exists
		else {
			ReadJsonFile(savePath, doc);
		}

		//Iterate through all system save actors
		for (auto actor : SceneDB::systemSaveActors) {
			if (!actor->removed && actor->dontDestroyOnLoad) {
				QueueSystemSave(actor, doc);
			}
			// System conceptually works with DDOL actors. They're persistent across scenes
			else if (!actor->removed && !actor->dontDestroyOnLoad) {
				std::cout << "Warning: Actor " << actor->GetID() << " (" << actor->GetName()
					<< ") marked Destroy on Load and System Save. Switch to Scene Save for proper behavior. Actor save voided." << std::endl;
			}
		}

		// Write system.save to temp
		TempSystemSave(doc);
	}
}

void DataManager::DumpTemp(int index) {
	if (index < 1 || index > DataManager::num_save_index) {
		std::cout << "error: saving to an invalid index" << std::endl;
		return;
	}
	else {
		std::string dirPath = "saves/" + std::to_string(index);
		std::filesystem::copy("saves/temp", dirPath, std::filesystem::copy_options::overwrite_existing
			| std::filesystem::copy_options::recursive);
		std::filesystem::remove_all("saves/temp");
		std::filesystem::create_directories("saves/temp");

		DataManager::last_index_accessed = index;

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
}

Actor* fetchActor(std::vector<Actor*> actors, int UUID) {
	for (auto actor : actors) {
		if (actor->GetID() == UUID) {
			return actor;
		}
	}

	return nullptr;
}

void DataManager::LoadScene() {
	std::string loadPath = "saves/" + std::to_string(DataManager::last_index_accessed) + "/" + SceneDB::currentScene + ".save";
	if (std::filesystem::exists(loadPath)) {
		rapidjson::Document doc;
		ReadJsonFile(loadPath, doc);

		for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
			auto& actor_vec = SceneDB::namedActors[itr->name.GetString()];
			auto& keys = itr->value;
			for (auto itr2 = keys.MemberBegin(); itr2 != keys.MemberEnd(); ++itr2) {
				std::string id_str = itr2->name.GetString();
				int id;
				std::from_chars(id_str.data(), id_str.data() + id_str.size(), id);
				Actor* actor = fetchActor(actor_vec, id);
				if (actor != nullptr) {
					auto& comps = itr2->value;
					for (auto itr3 = comps.MemberBegin(); itr3 != comps.MemberEnd(); ++itr3) {
						std::string comp_str = itr3->value.GetString();
						std::string comp_key = itr3->name.GetString();

						auto& comp = actor->keyedComponents[comp_key];
						luabridge::LuaRef savedComp = deserializeTable(comp_str);

						for (luabridge::Iterator it(savedComp); !it.isNil(); ++it) {
							(*comp)[it.key()] = it.value();
						}
					}
				}
			}
		}
	}

	loadPath = "saves/temp/" + SceneDB::currentScene + ".save";
	if (std::filesystem::exists(loadPath)) {
		rapidjson::Document doc;
		ReadJsonFile(loadPath, doc);

		for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
			auto& actor_vec = SceneDB::namedActors[itr->name.GetString()];
			auto& keys = itr->value;
			for (auto itr2 = keys.MemberBegin(); itr2 != keys.MemberEnd(); ++itr2) {
				std::string id_str = itr2->name.GetString();
				int id;
				std::from_chars(id_str.data(), id_str.data() + id_str.size(), id);
				Actor* actor = fetchActor(actor_vec, id);
				if (actor != nullptr) {
					auto& comps = itr2->value;
					for (auto itr3 = comps.MemberBegin(); itr3 != comps.MemberEnd(); ++itr3) {
						std::string comp_str = itr3->value.GetString();
						std::string comp_key = itr3->name.GetString();

						auto& comp = actor->keyedComponents[comp_key];
						luabridge::LuaRef savedComp = deserializeTable(comp_str);

						for (luabridge::Iterator it(savedComp); !it.isNil(); ++it) {
							(*comp)[it.key()] = it.value();
						}
					}
				}
			}
		}
	}
}

void DataManager::LoadSystem() {
	std::string loadPath = "saves/" + std::to_string(DataManager::last_index_accessed) + "/system.save";
	if (std::filesystem::exists(loadPath)) {
		rapidjson::Document doc;
		ReadJsonFile(loadPath, doc);

		for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
			if (!itr->value.IsObject()) {
				continue;
			}
			auto& actor_vec = SceneDB::namedActors[itr->name.GetString()];
			auto& keys = itr->value;
			for (auto itr2 = keys.MemberBegin(); itr2 != keys.MemberEnd(); ++itr2) {
				std::string id_str = itr2->name.GetString();
				int id;
				std::from_chars(id_str.data(), id_str.data() + id_str.size(), id);
				Actor* actor = fetchActor(actor_vec, id);
				if (actor != nullptr) {
					auto& comps = itr2->value;
					for (auto itr3 = comps.MemberBegin(); itr3 != comps.MemberEnd(); ++itr3) {
						std::string comp_str = itr3->value.GetString();
						std::string comp_key = itr3->name.GetString();

						auto& comp = actor->keyedComponents[comp_key];
						luabridge::LuaRef savedComp = deserializeTable(comp_str);

						for (luabridge::Iterator it(savedComp); !it.isNil(); ++it) {
							(*comp)[it.key()] = it.value();
						}
					}

					actor->SystemSave();
				}
				else {
					Actor* tempActor = new Actor();
					tempActor->SetName(itr->name.GetString());
					tempActor->SetID(id);

					auto& comps = itr2->value;
					for (auto itr3 = comps.MemberBegin(); itr3 != comps.MemberEnd(); ++itr3) {
						std::string comp_str = itr3->value.GetString();
						std::string comp_key = itr3->name.GetString();

						luabridge::LuaRef savedComp = deserializeTable(comp_str);

						ComponentDB::LoadSystemComponent(tempActor, savedComp["type"].cast<std::string>(), comp_key);

						auto& comp = tempActor->keyedComponents[comp_key];

						for (luabridge::Iterator it(savedComp); !it.isNil(); ++it) {
							(*comp)[it.key()] = it.value();
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

					tempActor->SystemSave();
				}
			}
		}
	}

	loadPath = "saves/temp/system.save";
	if (std::filesystem::exists(loadPath)) {
		rapidjson::Document doc;
		ReadJsonFile(loadPath, doc);

		for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
			auto& actor_vec = SceneDB::namedActors[itr->name.GetString()];
			auto& keys = itr->value;
			for (auto itr2 = keys.MemberBegin(); itr2 != keys.MemberEnd(); ++itr2) {
				std::string id_str = itr2->name.GetString();
				int id;
				std::from_chars(id_str.data(), id_str.data() + id_str.size(), id);
				Actor* actor = fetchActor(actor_vec, id);
				if (actor != nullptr) {
					auto& comps = itr2->value;
					for (auto itr3 = comps.MemberBegin(); itr3 != comps.MemberEnd(); ++itr3) {
						std::string comp_str = itr3->value.GetString();
						std::string comp_key = itr3->name.GetString();

						auto& comp = actor->keyedComponents[comp_key];
						luabridge::LuaRef savedComp = deserializeTable(comp_str);

						for (luabridge::Iterator it(savedComp); !it.isNil(); ++it) {
							(*comp)[it.key()] = it.value();
						}
					}

					actor->SystemSave();
				}
				else {
					Actor* tempActor = new Actor();
					tempActor->SetName(itr->name.GetString());
					tempActor->SetID(id);

					auto& comps = itr2->value;
					for (auto itr3 = comps.MemberBegin(); itr3 != comps.MemberEnd(); ++itr3) {
						std::string comp_str = itr3->value.GetString();
						std::string comp_key = itr3->name.GetString();

						luabridge::LuaRef savedComp = deserializeTable(comp_str);

						ComponentDB::LoadSystemComponent(tempActor, savedComp["type"].cast<std::string>(), comp_key);

						auto& comp = tempActor->keyedComponents[comp_key];

						for (luabridge::Iterator it(savedComp); !it.isNil(); ++it) {
							(*comp)[it.key()] = it.value();
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

					tempActor->SystemSave();
				}
			}
		}
	}
}