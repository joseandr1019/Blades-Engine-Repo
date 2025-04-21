#pragma once
#include <queue>
#include <string>
#include <unordered_map>

#include "SDL2_Img/SDL_image.h"

struct SceneImgStruct {
	int rotation_degrees = 0;
	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;
	int sorting_order = 0;
	float x;
	float y;
	float scale_x = 1.0f;
	float scale_y = 1.0f;
	float pivot_x = 0.5f;
	float pivot_y = 0.5f;
	SDL_Texture* img;
};

struct UIStruct {
	int x;
	int y;
	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;
	int sorting_order = 0;
	SDL_Texture* img;
};

struct PixStruct {
	int x;
	int y;
	int r;
	int g;
	int b;
	int a;
};

class ImageDB {
public:
	static inline std::deque<SceneImgStruct> sceneImgQueue;
	static inline std::deque<UIStruct> UIImgQueue;
	static inline std::queue<PixStruct> pixImgQueue;

	static inline std::unordered_map<std::string, SDL_Texture*> imageMap;

	static void LuaInit();

	static void LoadViewImage(SDL_Renderer* renderer, std::string& imageName, SDL_Texture*& image_ptr);
	static void CreateDefaultTextureWithName(const std::string& name);
	static void DrawEx(const std::string& image_name, float x, float y, float rotation_degrees, float scale_x, float scale_y,
		float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order);
private:

	ImageDB();
};