#include "Actor.h"
#include "ComponentDB.h"
#include "EngineUtils.h"
#include "TemplateDB.h"

#include <filesystem>
#include <iostream>
#include <string>

#include "rapidjson/document.h"

bool TemplateDB::DoesntDestroyOnLoad(const std::string& templateName) {
	if (TemplateDB::templateMap.find(templateName) != TemplateDB::templateMap.end()) {
		Actor* templateActor = &(TemplateDB::templateMap[templateName]);
		return templateActor->dontDestroyOnLoad;
	}
	else {
		return false;
	}
}

void TemplateDB::LoadTemplate(Actor& actor, const std::string& templateName) {
	if (TemplateDB::templateMap.find(templateName) != TemplateDB::templateMap.end()) {
		Actor* templateActor = &(TemplateDB::templateMap[templateName]);
		actor = Actor(*templateActor);
		ComponentDB::ComponentCopy(&actor, templateActor);
	}
	else {
		rapidjson::Document templateJson;
		std::string templatePath = "resources/actor_templates/" + templateName + ".template";
		if (!std::filesystem::exists(templatePath)) {
			std::cout << "error: template " + templateName + " is missing";
			std::exit(0);
		}

		ReadJsonFile(templatePath, templateJson);
		Actor templateActor = Actor();

		if (templateJson.HasMember("name")) {
			templateActor.SetName(std::string(templateJson["name"].GetString()));
		}

		if (templateJson.HasMember("components")) {
			for (auto itr = templateJson["components"].MemberBegin();
				itr != templateJson["components"].MemberEnd(); ++itr) {
				if (itr->value.HasMember("type")) {
					ComponentDB::LoadComponent(&templateActor,
						itr->value["type"].GetString(),
						itr->name.GetString(), itr);
				}
			}
		}

		actor = Actor(templateActor);
		ComponentDB::ComponentCopy(&actor, &templateActor);
		TemplateDB::templateMap[templateName] = templateActor;
	}
}