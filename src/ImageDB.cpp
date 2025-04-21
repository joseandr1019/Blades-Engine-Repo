#include "ComponentDB.h"
#include "ImageDB.h"
#include "Renderer.h"

#include "Helper.h"

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "SDL2_Img/SDL_image.h"

void DrawUI(const std::string& image_name, float x, float y) {
	UIStruct ui;
	ui.x = x;
	ui.y = y;

	try {
		ui.img = ImageDB::imageMap.at(image_name);
	}
	catch (const std::out_of_range&) {
		std::string imagePath = "resources/images/" + image_name + ".png";
		SDL_Texture* temp_ptr = IMG_LoadTexture(Renderer::renderer_ptr, imagePath.c_str());
		ui.img = temp_ptr;
		ImageDB::imageMap.insert(std::pair<std::string, SDL_Texture*>(image_name, temp_ptr));
	}

	ImageDB::UIImgQueue.push_back(ui);
}

void DrawUIEx(const std::string& image_name, float x, float y, float r, float g, float b, float a, float sorting_order) {
	UIStruct ui;
	ui.x = x;
	ui.y = y;
	ui.r = r;
	ui.g = g;
	ui.b = b;
	ui.a = a;
	ui.sorting_order = sorting_order;

	try {
		ui.img = ImageDB::imageMap.at(image_name);
	}
	catch (const std::out_of_range&) {
		std::string imagePath = "resources/images/" + image_name + ".png";
		SDL_Texture* temp_ptr = IMG_LoadTexture(Renderer::renderer_ptr, imagePath.c_str());
		ui.img = temp_ptr;
		ImageDB::imageMap.insert(std::pair<std::string, SDL_Texture*>(image_name, temp_ptr));
	}

	ImageDB::UIImgQueue.push_back(ui);
}

void Draw(const std::string& image_name, float x, float y) {
	SceneImgStruct sce;
	sce.x = x;
	sce.y = y;

	try {
		sce.img = ImageDB::imageMap.at(image_name);
	}
	catch (const std::out_of_range&) {
		std::string imagePath = "resources/images/" + image_name + ".png";
		SDL_Texture* temp_ptr = IMG_LoadTexture(Renderer::renderer_ptr, imagePath.c_str());
		sce.img = temp_ptr;
		ImageDB::imageMap.insert(std::pair<std::string, SDL_Texture*>(image_name, temp_ptr));
	}

	ImageDB::sceneImgQueue.push_back(sce);
}

void ImageDB::DrawEx(const std::string& image_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, 
	float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order) {
	SceneImgStruct sce;
	sce.x = x;
	sce.y = y;
	sce.rotation_degrees = rotation_degrees;
	sce.scale_x = scale_x;
	sce.scale_y = scale_y;
	sce.pivot_x = pivot_x;
	sce.pivot_y = pivot_y;
	sce.r = r;
	sce.g = g;
	sce.b = b;
	sce.a = a;
	sce.sorting_order = sorting_order;

	try {
		sce.img = ImageDB::imageMap.at(image_name);
	}
	catch (const std::out_of_range&) {
		std::string imagePath = "resources/images/" + image_name + ".png";
		SDL_Texture* temp_ptr = IMG_LoadTexture(Renderer::renderer_ptr, imagePath.c_str());
		sce.img = temp_ptr;
		ImageDB::imageMap.insert(std::pair<std::string, SDL_Texture*>(image_name, temp_ptr));
	}

	ImageDB::sceneImgQueue.push_back(sce);
}

void DrawPixel(float x, float y, float r, float g, float b, float a) {
	PixStruct pix;

	pix.x = x;
	pix.y = y;
	pix.r = r;
	pix.g = g;
	pix.b = b;
	pix.a = a;

	ImageDB::pixImgQueue.push(pix);
}

void ImageDB::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Image")
		.addFunction("DrawUI", &DrawUI)
		.addFunction("DrawUIEx", &DrawUIEx)
		.addFunction("Draw", &Draw)
		.addFunction("DrawEx", &ImageDB::DrawEx)
		.addFunction("DrawPixel", &DrawPixel)
		.endNamespace();
}

void ImageDB::LoadViewImage(SDL_Renderer* renderer, std::string& imageName, SDL_Texture*& image_ptr) {
	if (ImageDB::imageMap.find(imageName) == ImageDB::imageMap.end()) {
		std::string imagePath = "resources/images/" + imageName + ".png";
		SDL_Texture* temp_ptr = IMG_LoadTexture(renderer, imagePath.c_str());
		image_ptr = temp_ptr;
		ImageDB::imageMap.insert(std::pair<std::string, SDL_Texture*>(imageName, temp_ptr));
	}
}

void ImageDB::CreateDefaultTextureWithName(const std::string& name) {
	if (imageMap.find(name) != imageMap.end()) {
		return;
	}

	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);

	Uint32 white_color = SDL_MapRGBA(surf->format, 255, 255, 255, 255);
	SDL_FillRect(surf, NULL, white_color);

	SDL_Texture* text = SDL_CreateTextureFromSurface(Renderer::renderer_ptr, surf);

	SDL_FreeSurface(surf);
	imageMap[name] = text;
}