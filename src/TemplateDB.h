#pragma once
#include "Actor.h"

#include <string>
#include <unordered_map>

class TemplateDB {
public:
	static void LoadTemplate(Actor& actor, const std::string& templateName);
	static bool DoesntDestroyOnLoad(const std::string& templateName);
private:
	static inline std::unordered_map<std::string, Actor> templateMap;
	TemplateDB() {}
};