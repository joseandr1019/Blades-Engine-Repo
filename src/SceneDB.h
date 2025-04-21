#pragma once
#include "Actor.h"

#include <string>
#include <vector>
#include <unordered_map>

#include "box2d/box2d.h"

class SceneDB {
public:
	static bool nowRemoving;
	static bool nowTrimming;
	static bool nowClearing;
	static std::string currentScene;
	static std::string nextScene;
	static inline std::vector<Actor*> actors;
	static inline std::unordered_map<std::string, std::vector<Actor*>> namedActors;
	static inline std::vector<Actor*> startingActors;
	static inline std::vector<Actor*> updatingActors;
	static inline std::vector<Actor*> lateUpdatingActors;
	static inline std::vector<Actor*> compAddedActors;
	static inline std::vector<Actor*> compRemovedActors;
	static inline std::vector<Actor*> addedActors;
	static inline std::vector<Actor*> removedActors;
	static inline std::vector<Actor*> willCompRemoveActors;
	static inline std::vector<Actor*> willRemoveActors;
	static inline std::vector<Actor*> retainedActors;
	static inline std::vector<Actor*> sceneSaveActors;
	static inline std::vector<Actor*> systemSaveActors;
	static b2World world;
	static inline std::unordered_map<std::string, std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>> event_bus;
	static inline std::unordered_map<std::string, std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>> pending_subscriptions;
	static inline std::unordered_map<std::string, std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>> pending_unsubscriptions;

	static int UUID;

	static void LuaInit();

	static void ActorInsertSort(std::vector<Actor*>& actors);
	static void LoadScene(std::string& sceneName);
	static void AddComponents();
	static void RemoveComponents();
	static void AddActors();
	static void RemoveActors();
	static void EventSubs();
private:
	SceneDB() {}
};