#pragma once
#include "Actor.h"

#include "rapidjson/rapidjson.h"

class DataManager {
public:
	static int num_save_index;
	static int last_index_accessed;
	static bool loadingSave;

	void static SaveScene();
	void static SaveSystem();
	void static LoadScene();
	void static LoadSystem();
	void static LoadConfig();
	void static LuaInit();

	void static InitSaves();
	void static QueueSceneSave(Actor* actor, rapidjson::Document& doc);
	void static QueueSystemSave(Actor* actor, rapidjson::Document& doc);
	void static TempSceneSave(rapidjson::Document& doc);
	void static TempSystemSave(rapidjson::Document& doc);
	void static DumpTemp(int index);
private:
	DataManager();
};