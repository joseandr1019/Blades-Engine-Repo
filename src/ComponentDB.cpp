#include "Actor.h"
#include "AudioDB.h"
#include "ComponentDB.h"
#include "DataManager.h"
#include "ImageDB.h"
#include "InputManager.h"
#include "ParticleSystem.h"
#include "Renderer.h"
#include "Rigidbody.h"
#include "SceneDB.h"
#include "TextDB.h"

#include "Helper.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>

#include "box2d/box2d.h"
#include "glm/glm.hpp"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "rapidjson/document.h"

lua_State* ComponentDB::lua_state = nullptr;
std::unordered_map<std::string, std::optional<luabridge::LuaRef>> ComponentDB::componentCache;
int ComponentDB::runtimeAddCount = 0;

void CppDebugLog(const std::string& message) {
	std::cout << message << '\n';
}

void Quit() {
	std::exit(0);
}

void Sleep(const int dur_ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(dur_ms));
}

int GetFrame() {
	return Helper::GetFrameNumber();
}

void OpenURL(const std::string& url) {
	#ifdef _WIN32
		std::string command = "start " + url;
		std::system(command.c_str());
	#elif __APPLE__
		std::string command = "open " + url;
		std::system(command.c_str());
	#else
		std::string command = "xdg-open " + url;
		std::system(command.c_str());
	#endif
}

std::string ReturnVec2Type() {
	return "vec2";
}

std::string ReturnVector2Type() {
	return "Vector2";
}

void ComponentDB::LuaInit() {
	ComponentDB::lua_state = luaL_newstate();
	luaL_openlibs(ComponentDB::GetLuaState());

	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Debug")
		.addFunction("Log", &CppDebugLog)
		.endNamespace();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Application")
		.addFunction("Quit", &Quit)
		.addFunction("Sleep", &Sleep)
		.addFunction("GetFrame", &GetFrame)
		.addFunction("OpenURL", &OpenURL)
		.endNamespace();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginClass<glm::vec2>("vec2")
		.addProperty("x", &glm::vec2::x)
		.addProperty("y", &glm::vec2::y)
		.addStaticFunction("getType", &ReturnVec2Type)
		.endClass();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginClass<b2Vec2>("Vector2")
		.addConstructor<void(*) (float, float)>()
		.addProperty("x", &b2Vec2::x)
		.addProperty("y", &b2Vec2::y)
		.addFunction("__add", &b2Vec2::operator_add)
		.addFunction("__sub", &b2Vec2::operator_sub)
		.addFunction("__mul", &b2Vec2::operator_mul)
		.addFunction("Normalize", &b2Vec2::Normalize)
		.addFunction("Length", &b2Vec2::Length)
		.addStaticFunction("Distance", &b2Distance)
		.addStaticFunction("Dot", static_cast<float(*) (const b2Vec2&, const b2Vec2&)>(&b2Dot))
		.addStaticFunction("getType", &ReturnVector2Type)
		.endClass();

	Actor::LuaInit();
	SceneDB::LuaInit();
	Input::LuaInit();
	TextDB::LuaInit();
	AudioDB::LuaInit();
	ImageDB::LuaInit();
	Renderer::LuaInit();
	Rigidbody::LuaInit();
	ParticleSystem::LuaInit();
	DataManager::LuaInit();
}

void ComponentDB::ReportError(const std::string& actor_name, const luabridge::LuaException& e) {
	std::string error_message = e.what();
	std::replace(error_message.begin(), error_message.end(), '\\', '/');

	std::cout << "\033[31m" << actor_name << " : "
		<< error_message << "\033[0m" << std::endl;
}

void ComponentDB::ComponentInsertSort(std::vector<std::shared_ptr<luabridge::LuaRef>>& components) {
	for (auto itr = components.end() - 1; itr != components.begin(); --itr) {
		std::string key1 = (*(*itr))["key"].cast<std::string>();
		std::string key2 = (*(*(itr - 1)))["key"].cast<std::string>();
		if (key1 < key2) {
			auto temp = *itr;
			*itr = *(itr - 1);
			*(itr - 1) = temp;
		}
	}
}

void ComponentDB::LoadComponent(Actor* actor, const std::string& component, const std::string& key,
	rapidjson::GenericMemberIterator<false, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> value) {
	if (component != "Rigidbody" && component != "ParticleSystem") {
		luabridge::LuaRef parentTable = luabridge::LuaRef(ComponentDB::GetLuaState());

		try {
			parentTable = ComponentDB::componentCache.at(component).value();
		}
		catch (const std::out_of_range&) {
			std::string componentPath = "resources/component_types/" + component + ".lua";
			if (!std::filesystem::exists(componentPath)) {
				std::cout << "error: failed to locate component "
					<< component;
				std::exit(0);
			}
			if (luaL_dofile(ComponentDB::GetLuaState(), componentPath.c_str()) != LUA_OK) {
				std::cout << "problem with lua file " << component;
				std::exit(0);
			}
			ComponentDB::componentCache[component] = luabridge::getGlobal(ComponentDB::GetLuaState(), component.c_str());
			parentTable = ComponentDB::componentCache.at(component).value();
		}
		auto instanceTable = std::make_shared<luabridge::LuaRef>(luabridge::newTable(ComponentDB::GetLuaState()));
		ComponentDB::EstablishInheritance(*instanceTable, parentTable);
		(*instanceTable)["key"] = key;
		(*instanceTable)["type"] = component;
		(*instanceTable)["enabled"] = true;
		(*instanceTable)["removed"] = false;

		for (auto itr = value->value.MemberBegin();
			itr != value->value.MemberEnd(); ++itr) {
			if (std::string(itr->name.GetString()) == "type") {
				continue;
			}
			else {
				ComponentDB::LoadOverride(*instanceTable, itr);
			}
		}
		actor->InjectConvenienceReference(instanceTable);

		actor->keyedComponents.insert(std::pair(key, instanceTable));
		actor->typedComponents[component].push_back(instanceTable);
		ComponentDB::ComponentInsertSort(actor->typedComponents[component]);

		if ((*instanceTable)["OnStart"].isFunction()) {
			actor->startingComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->startingComponents);
		}

		if ((*instanceTable)["OnDestroy"].isFunction()) {
			actor->destroyingComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->destroyingComponents);
		}

		if ((*instanceTable)["OnUpdate"].isFunction()) {
			actor->updatingComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->updatingComponents);
		}

		if ((*instanceTable)["OnLateUpdate"].isFunction()) {
			actor->lateUpdatingComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->lateUpdatingComponents);
		}

		if ((*instanceTable)["OnCollisionEnter"].isFunction()) {
			actor->collisionEnterComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->collisionEnterComponents);
		}

		if ((*instanceTable)["OnCollisionExit"].isFunction()) {
			actor->collisionExitComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->collisionExitComponents);
		}

		if ((*instanceTable)["OnTriggerEnter"].isFunction()) {
			actor->triggerEnterComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->triggerEnterComponents);
		}

		if ((*instanceTable)["OnTriggerExit"].isFunction()) {
			actor->triggerExitComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->triggerExitComponents);
		}
	}
	else if (component == "Rigidbody") {
		Rigidbody* temp = new Rigidbody();
		luabridge::LuaRef ref(ComponentDB::GetLuaState(), temp);
		auto component = std::make_shared<luabridge::LuaRef>(ref);
		temp->key = key;
		temp->actor = actor;

		for (auto itr = value->value.MemberBegin();
			itr != value->value.MemberEnd(); ++itr) {
			if (std::string(itr->name.GetString()) == "type") {
				continue;
			}
			else {
				ComponentDB::LoadOverride(*component, itr);
			}
		}

		actor->keyedComponents.insert(std::pair(key, component));
		actor->typedComponents[temp->type].push_back(component);
		ComponentInsertSort(actor->typedComponents[temp->type]);

		actor->startingComponents.push_back(component);
		ComponentInsertSort(actor->startingComponents);
		actor->destroyingComponents.push_back(component);
		ComponentInsertSort(actor->destroyingComponents);
	}
	else if (component == "ParticleSystem") {
		ParticleSystem* temp = new ParticleSystem();
		luabridge::LuaRef ref(ComponentDB::GetLuaState(), temp);
		auto component = std::make_shared<luabridge::LuaRef>(ref);
		temp->key = key;
		temp->actor = actor;

		for (auto itr = value->value.MemberBegin();
			itr != value->value.MemberEnd(); ++itr) {
			if (std::string(itr->name.GetString()) == "type") {
				continue;
			}
			else {
				ComponentDB::LoadOverride(*component, itr);
			}
		}

		actor->keyedComponents.insert(std::pair(key, component));
		actor->typedComponents[temp->type].push_back(component);
		ComponentInsertSort(actor->typedComponents[temp->type]);

		actor->startingComponents.push_back(component);
		ComponentInsertSort(actor->startingComponents);
		actor->updatingComponents.push_back(component);
		ComponentInsertSort(actor->updatingComponents);
	}
}

void ComponentDB::LoadSystemComponent(Actor* actor, const std::string& component, const std::string& key) {
	if (component != "Rigidbody" && component != "ParticleSystem") {
		luabridge::LuaRef parentTable = luabridge::LuaRef(ComponentDB::GetLuaState());

		try {
			parentTable = ComponentDB::componentCache.at(component).value();
		}
		catch (const std::out_of_range&) {
			std::string componentPath = "resources/component_types/" + component + ".lua";
			if (!std::filesystem::exists(componentPath)) {
				std::cout << "error: failed to locate component "
					<< component;
				std::exit(0);
			}
			if (luaL_dofile(ComponentDB::GetLuaState(), componentPath.c_str()) != LUA_OK) {
				std::cout << "problem with lua file " << component;
				std::exit(0);
			}
			ComponentDB::componentCache[component] = luabridge::getGlobal(ComponentDB::GetLuaState(), component.c_str());
			parentTable = ComponentDB::componentCache.at(component).value();
		}
		auto instanceTable = std::make_shared<luabridge::LuaRef>(luabridge::newTable(ComponentDB::GetLuaState()));
		ComponentDB::EstablishInheritance(*instanceTable, parentTable);
		(*instanceTable)["key"] = key;
		(*instanceTable)["type"] = component;
		(*instanceTable)["enabled"] = true;
		(*instanceTable)["removed"] = false;

		actor->InjectConvenienceReference(instanceTable);

		actor->keyedComponents.insert(std::pair(key, instanceTable));
		actor->typedComponents[component].push_back(instanceTable);
		ComponentDB::ComponentInsertSort(actor->typedComponents[component]);

		if ((*instanceTable)["OnStart"].isFunction()) {
			actor->startingComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->startingComponents);
		}

		if ((*instanceTable)["OnDestroy"].isFunction()) {
			actor->destroyingComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->destroyingComponents);
		}

		if ((*instanceTable)["OnUpdate"].isFunction()) {
			actor->updatingComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->updatingComponents);
		}

		if ((*instanceTable)["OnLateUpdate"].isFunction()) {
			actor->lateUpdatingComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->lateUpdatingComponents);
		}

		if ((*instanceTable)["OnCollisionEnter"].isFunction()) {
			actor->collisionEnterComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->collisionEnterComponents);
		}

		if ((*instanceTable)["OnCollisionExit"].isFunction()) {
			actor->collisionExitComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->collisionExitComponents);
		}

		if ((*instanceTable)["OnTriggerEnter"].isFunction()) {
			actor->triggerEnterComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->triggerEnterComponents);
		}

		if ((*instanceTable)["OnTriggerExit"].isFunction()) {
			actor->triggerExitComponents.push_back(instanceTable);
			ComponentDB::ComponentInsertSort(actor->triggerExitComponents);
		}
	}
	else if (component == "Rigidbody") {
		Rigidbody* temp = new Rigidbody();
		luabridge::LuaRef ref(ComponentDB::GetLuaState(), temp);
		auto component = std::make_shared<luabridge::LuaRef>(ref);
		temp->key = key;
		temp->actor = actor;

		actor->keyedComponents.insert(std::pair(key, component));
		actor->typedComponents[temp->type].push_back(component);
		ComponentInsertSort(actor->typedComponents[temp->type]);

		actor->startingComponents.push_back(component);
		ComponentInsertSort(actor->startingComponents);
		actor->destroyingComponents.push_back(component);
		ComponentInsertSort(actor->destroyingComponents);
	}
	else if (component == "ParticleSystem") {
		ParticleSystem* temp = new ParticleSystem();
		luabridge::LuaRef ref(ComponentDB::GetLuaState(), temp);
		auto component = std::make_shared<luabridge::LuaRef>(ref);
		temp->key = key;
		temp->actor = actor;

		actor->keyedComponents.insert(std::pair(key, component));
		actor->typedComponents[temp->type].push_back(component);
		ComponentInsertSort(actor->typedComponents[temp->type]);

		actor->startingComponents.push_back(component);
		ComponentInsertSort(actor->startingComponents);
		actor->updatingComponents.push_back(component);
		ComponentInsertSort(actor->updatingComponents);
	}
}

void ComponentDB::EstablishInheritance(luabridge::LuaRef& instanceTable, luabridge::LuaRef& parentTable) {
	luabridge::LuaRef newMetatable = luabridge::newTable(ComponentDB::GetLuaState());
	newMetatable["__index"] = parentTable;

	instanceTable.push(ComponentDB::GetLuaState());
	newMetatable.push(ComponentDB::GetLuaState());
	lua_setmetatable(ComponentDB::GetLuaState(), -2);
	lua_pop(ComponentDB::GetLuaState(), 1);
}

void ComponentDB::ComponentCopy(Actor* actor, Actor* templateActor) {
	for (auto& component : templateActor->keyedComponents) {
		if (!((*component.second).isUserdata())) {
			auto instanceTable = std::make_shared<luabridge::LuaRef>(luabridge::newTable(ComponentDB::GetLuaState()));
			EstablishInheritance(*instanceTable, *(component.second));
			actor->InjectConvenienceReference(instanceTable);
			actor->keyedComponents.insert(
				std::pair((*instanceTable)["key"].cast<std::string>(), instanceTable));
			actor->typedComponents[(*instanceTable)["type"].cast<std::string>()].push_back(instanceTable);

			if ((*instanceTable)["OnStart"].isFunction()) {
				actor->startingComponents.push_back(instanceTable);
				ComponentDB::ComponentInsertSort(actor->startingComponents);
			}

			if ((*instanceTable)["OnUpdate"].isFunction()) {
				actor->updatingComponents.push_back(instanceTable);
				ComponentDB::ComponentInsertSort(actor->updatingComponents);
			}

			if ((*instanceTable)["OnLateUpdate"].isFunction()) {
				actor->lateUpdatingComponents.push_back(instanceTable);
				ComponentDB::ComponentInsertSort(actor->lateUpdatingComponents);
			}

			if ((*instanceTable)["OnCollisionEnter"].isFunction()) {
				actor->collisionEnterComponents.push_back(instanceTable);
				ComponentDB::ComponentInsertSort(actor->collisionEnterComponents);
			}

			if ((*instanceTable)["OnCollisionExit"].isFunction()) {
				actor->collisionExitComponents.push_back(instanceTable);
				ComponentDB::ComponentInsertSort(actor->collisionExitComponents);
			}

			if ((*instanceTable)["OnTriggerEnter"].isFunction()) {
				actor->triggerEnterComponents.push_back(instanceTable);
				ComponentDB::ComponentInsertSort(actor->triggerEnterComponents);
			}

			if ((*instanceTable)["OnTriggerExit"].isFunction()) {
				actor->triggerExitComponents.push_back(instanceTable);
				ComponentDB::ComponentInsertSort(actor->triggerExitComponents);
			}

			if ((*instanceTable)["OnDestroy"].isFunction()) {
				actor->destroyingComponents.push_back(instanceTable);
				ComponentDB::ComponentInsertSort(actor->destroyingComponents);
			}
		}
		else if ((*component.second)["type"].cast<std::string>() == "Rigidbody") {
			Rigidbody* rb = new Rigidbody((*(component.second)).cast<Rigidbody*>());
			luabridge::LuaRef ref(ComponentDB::GetLuaState(), rb);
			auto refComp = std::make_shared<luabridge::LuaRef>(ref);
			rb->actor = actor;

			actor->keyedComponents.insert(std::pair(rb->key, refComp));
			actor->typedComponents[rb->type].push_back(refComp);
			ComponentInsertSort(actor->typedComponents[rb->type]);

			actor->startingComponents.push_back(refComp);
			ComponentInsertSort(actor->startingComponents);
			actor->destroyingComponents.push_back(refComp);
			ComponentInsertSort(actor->destroyingComponents);
		}
		else if ((*component.second)["type"].cast<std::string>() == "ParticleSystem") {
			ParticleSystem* ps = new ParticleSystem((*(component.second)).cast<ParticleSystem*>());
			luabridge::LuaRef ref(ComponentDB::GetLuaState(), ps);
			auto refComp = std::make_shared<luabridge::LuaRef>(ref);
			ps->actor = actor;

			actor->keyedComponents.insert(std::pair(ps->key, refComp));
			actor->typedComponents[ps->type].push_back(refComp);
			ComponentInsertSort(actor->typedComponents[ps->type]);

			actor->startingComponents.push_back(refComp);
			ComponentInsertSort(actor->startingComponents);
			actor->updatingComponents.push_back(refComp);
			ComponentInsertSort(actor->updatingComponents);
		}
	}
}

void ComponentDB::LoadOverride(luabridge::LuaRef component,
	rapidjson::GenericMemberIterator<false, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> value) {
	if (value->value.IsInt()) {
		component[value->name.GetString()] = value->value.GetInt();
	}

	else if (value->value.IsFloat()) {
		component[value->name.GetString()] = value->value.GetFloat();
	}

	else if (value->value.IsString()) {

		component[value->name.GetString()] = value->value.GetString();
	}

	else if (value->value.IsBool()) {
		component[value->name.GetString()] = value->value.GetBool();
	}
}

std::shared_ptr<luabridge::LuaRef> ComponentDB::RuntimeComponentLoad(Actor* actor, const std::string& component) {
	if (component != "Rigidbody") {
		luabridge::LuaRef parentTable = luabridge::LuaRef(ComponentDB::GetLuaState());

		try {
			parentTable = ComponentDB::componentCache.at(component).value();
		}
		catch (const std::out_of_range&) {
			std::string componentPath = "resources/component_types/" + component + ".lua";
			if (!std::filesystem::exists(componentPath)) {
				std::cout << "error: failed to locate component "
					<< component;
				std::exit(0);
			}
			if (luaL_dofile(ComponentDB::GetLuaState(), componentPath.c_str()) != LUA_OK) {
				std::cout << "problem with lua file " << component;
				std::exit(0);
			}
			ComponentDB::componentCache[component] = luabridge::getGlobal(ComponentDB::GetLuaState(), component.c_str());
			parentTable = ComponentDB::componentCache.at(component).value();
		}
		auto instanceTable = std::make_shared<luabridge::LuaRef>(luabridge::newTable(ComponentDB::GetLuaState()));
		ComponentDB::EstablishInheritance(*instanceTable, parentTable);
		std::string key = "r" + std::to_string(ComponentDB::runtimeAddCount);
		ComponentDB::runtimeAddCount++;
		(*instanceTable)["key"] = key;
		(*instanceTable)["type"] = component;
		(*instanceTable)["enabled"] = true;
		(*instanceTable)["removed"] = false;
		actor->InjectConvenienceReference(instanceTable);

		actor->addedComponents.push_back(instanceTable);

		return instanceTable;
	}
	else if (component == "Rigidbody") {
		Rigidbody* temp = new Rigidbody();
		luabridge::LuaRef ref(ComponentDB::GetLuaState(), temp);
		auto component = std::make_shared<luabridge::LuaRef>(ref);
		std::string key = "r" + std::to_string(ComponentDB::runtimeAddCount);
		ComponentDB::runtimeAddCount++;
		temp->key = key;
		temp->actor = actor;

		actor->keyedComponents.insert(std::pair(key, component));
		actor->typedComponents[temp->type].push_back(component);
		ComponentInsertSort(actor->typedComponents[temp->type]);

		actor->startingComponents.push_back(component);
		ComponentInsertSort(actor->startingComponents);
		actor->destroyingComponents.push_back(component);
		ComponentInsertSort(actor->destroyingComponents);

		return component;
	}
	else if (component == "ParticleSystem") {
		ParticleSystem* temp = new ParticleSystem();
		luabridge::LuaRef ref(ComponentDB::GetLuaState(), temp);
		auto component = std::make_shared<luabridge::LuaRef>(ref);
		std::string key = "r" + std::to_string(ComponentDB::runtimeAddCount);
		ComponentDB::runtimeAddCount++;
		temp->key = key;
		temp->actor = actor;

		actor->keyedComponents.insert(std::pair(key, component));
		actor->typedComponents[temp->type].push_back(component);
		ComponentInsertSort(actor->typedComponents[temp->type]);

		actor->startingComponents.push_back(component);
		ComponentInsertSort(actor->startingComponents);
		actor->updatingComponents.push_back(component);
		ComponentInsertSort(actor->updatingComponents);

		return component;
	}
    else {
        return luabridge::LuaRef(ComponentDB::GetLuaState());
    }
}
