#pragma once
#include "Actor.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "rapidjson/document.h"

class ComponentDB {
public:
	static std::unordered_map<std::string, std::optional<luabridge::LuaRef>> componentCache;

	static inline lua_State* GetLuaState() {
		return ComponentDB::lua_state;
	}
	static void LuaInit();
	static void ReportError(const std::string& actor_name, const luabridge::LuaException& e);

	static void ComponentInsertSort(std::vector<std::shared_ptr<luabridge::LuaRef>>& components);
	static void LoadComponent(Actor* actor, const std::string& component, const std::string& key,
		rapidjson::GenericMemberIterator<false, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> value);
	static void LoadSystemComponent(Actor* actor, const std::string& component, const std::string& key);
	static void EstablishInheritance(luabridge::LuaRef& instanceTable, luabridge::LuaRef& parentTable);
	static void LoadOverride(luabridge::LuaRef component,
		rapidjson::GenericMemberIterator<false, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> value);
	static void ComponentCopy(Actor* actor, Actor* templateActor);
	static std::shared_ptr<luabridge::LuaRef> RuntimeComponentLoad(Actor* actor, const std::string& component);
private:
	static lua_State* lua_state;
	static int runtimeAddCount;
	ComponentDB();
};