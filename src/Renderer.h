#pragma once
#include <string>

#include "glm/glm.hpp"
#include "SDL2_Img/SDL_image.h"

class Renderer {
public:
	static const int UNIT_TO_PIXELS_CONVERSION = 100; // Units are now meters
	static float RENDER_SCALE;
	static const SDL_Color WHITE;
	static SDL_Color CLEAR_COLOR;
	static SDL_Renderer* renderer_ptr;
	static glm::ivec2 WINDOW_CENTER;
	static glm::ivec2 WINDOW_RESOLUTION;
	static glm::vec2 cameraPos;
	static std::string GAME_TITLE;

	static void LuaInit();
	static void RenderRenderer();
	static void SetCameraWidth(const int x_resolution);
	static void SetCameraHeight(const int y_resolution);
private:
	Renderer() {}
};