#include "ComponentDB.h"
#include "ImageDB.h"
#include "Renderer.h"
#include "SceneDB.h"
#include "TextDB.h"

#include "Helper.h"

#include <string>

#include "glm/glm.hpp"
#include "SDL2/SDL.h"
#include "SDL2_Img/SDL_image.h"

const SDL_Color Renderer::WHITE = { 255, 255, 255, 255 };
glm::vec2 Renderer::cameraPos = glm::vec2(0.0f, 0.0f);
SDL_Color Renderer::CLEAR_COLOR = Renderer::WHITE;
std::string Renderer::GAME_TITLE = "";
float Renderer::RENDER_SCALE = 1.0f;
SDL_Renderer* Renderer::renderer_ptr = nullptr;
glm::ivec2 Renderer::WINDOW_CENTER = glm::ivec2(320, 180);
glm::ivec2 Renderer::WINDOW_RESOLUTION = glm::ivec2(640, 360);

void SetPosition(float x, float y) {
	Renderer::cameraPos.x = x;
	Renderer::cameraPos.y = y;
}

float GetPositionX() {
	return Renderer::cameraPos.x;
}

float GetPositionY() {
	return Renderer::cameraPos.y;
}

void SetZoom(float zoom_factor) {
	Renderer::RENDER_SCALE = zoom_factor;
}

float GetZoom() {
	return Renderer::RENDER_SCALE;
}

void Renderer::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Camera")
		.addFunction("SetPosition", &SetPosition)
		.addFunction("GetPositionX", &GetPositionX)
		.addFunction("GetPositionY", &GetPositionY)
		.addFunction("SetZoom", &SetZoom)
		.addFunction("GetZoom", &GetZoom)
		.endNamespace();
}

void Renderer::SetCameraWidth(int x_resolution) {
	Renderer::WINDOW_RESOLUTION.x = x_resolution;
	Renderer::WINDOW_CENTER.x = x_resolution / 2;
}

void Renderer::SetCameraHeight(int y_resolution) {
	Renderer::WINDOW_RESOLUTION.y = y_resolution;
	Renderer::WINDOW_CENTER.y = y_resolution / 2;
}

bool compareUIRequests(const UIStruct& a, const UIStruct& b) {
	return a.sorting_order < b.sorting_order;
}

bool compareSceneRequests(const SceneImgStruct& a, const SceneImgStruct& b) {
	return a.sorting_order < b.sorting_order;
}

void Renderer::RenderRenderer() {
	SDL_SetRenderDrawColor(Renderer::renderer_ptr, Renderer::CLEAR_COLOR.r,
		Renderer::CLEAR_COLOR.g, Renderer::CLEAR_COLOR.b, SDL_ALPHA_TRANSPARENT);
	SDL_RenderClear(Renderer::renderer_ptr);

	std::stable_sort(ImageDB::sceneImgQueue.begin(), ImageDB::sceneImgQueue.end(), compareSceneRequests);

	SDL_RenderSetScale(Renderer::renderer_ptr, Renderer::RENDER_SCALE, Renderer::RENDER_SCALE);
	while (!ImageDB::sceneImgQueue.empty()) {
		auto& img = ImageDB::sceneImgQueue.front();

		float rel_unit_x_pos = img.x - Renderer::cameraPos.x;
		float rel_unit_y_pos = img.y - Renderer::cameraPos.y;

		SDL_FRect img_rect = SDL_FRect();
		Helper::SDL_QueryTexture(img.img, &img_rect.w, &img_rect.h);


		SDL_RendererFlip flag = SDL_FLIP_NONE;
		if (img.scale_x < 0) flag = SDL_RendererFlip(flag | SDL_FLIP_HORIZONTAL);
		if (img.scale_y < 0) flag = SDL_RendererFlip(flag | SDL_FLIP_VERTICAL);

		img_rect.w *= glm::abs(img.scale_x);
		img_rect.h *= glm::abs(img.scale_y);

		SDL_FPoint img_piv = { (img.pivot_x * img_rect.w), (img.pivot_y * img_rect.h) };

		img_rect.x = (rel_unit_x_pos * UNIT_TO_PIXELS_CONVERSION + Renderer::WINDOW_CENTER.x / Renderer::RENDER_SCALE - img_piv.x);
		img_rect.y = (rel_unit_y_pos * UNIT_TO_PIXELS_CONVERSION + Renderer::WINDOW_CENTER.y / Renderer::RENDER_SCALE - img_piv.y);

		SDL_SetTextureColorMod(img.img, img.r, img.g, img.b);
		SDL_SetTextureAlphaMod(img.img, img.a);
		Helper::SDL_RenderCopyEx(0, "", Renderer::renderer_ptr, img.img, NULL, &img_rect, img.rotation_degrees, &img_piv, flag);
		SDL_RenderSetScale(Renderer::renderer_ptr, Renderer::RENDER_SCALE, Renderer::RENDER_SCALE);
		SDL_SetTextureAlphaMod(img.img, 255);
		SDL_SetTextureColorMod(img.img, 255, 255, 255);

		ImageDB::sceneImgQueue.pop_front();
	}

	SDL_RenderSetScale(Renderer::renderer_ptr, 1, 1);

	std::stable_sort(ImageDB::UIImgQueue.begin(), ImageDB::UIImgQueue.end(), compareUIRequests);
	while (!ImageDB::UIImgQueue.empty()) {
		auto& img = ImageDB::UIImgQueue.front();
		SDL_FRect rect;
		rect.x = img.x;
		rect.y = img.y;
		
		Helper::SDL_QueryTexture(img.img, &rect.w, &rect.h);
		SDL_SetTextureColorMod(img.img, img.r, img.g, img.b);
		SDL_SetTextureAlphaMod(img.img, img.a);
		Helper::SDL_RenderCopyEx(0, "", Renderer::renderer_ptr, img.img, NULL, &rect, 0.0f, NULL, SDL_FLIP_NONE);
		SDL_SetTextureAlphaMod(img.img, 255);
		SDL_SetTextureColorMod(img.img, 255, 255, 255);

		ImageDB::UIImgQueue.pop_front();
	}

	while (!TextDB::textDrawQueue.empty()) {
		auto& tex = TextDB::textDrawQueue.front();
		auto surf = TTF_RenderText_Solid(tex.font, tex.content.c_str(), tex.color);
		auto text = SDL_CreateTextureFromSurface(Renderer::renderer_ptr, surf);
		SDL_FreeSurface(surf);
		SDL_FRect rect;
		rect.x = tex.x;
		rect.y = tex.y;
		Helper::SDL_QueryTexture(text, &rect.w, &rect.h);
		Helper::SDL_RenderCopyEx(0, "", Renderer::renderer_ptr, text, NULL, &rect, 0.0f, NULL, SDL_FLIP_NONE);
		TextDB::textDrawQueue.pop();
	}

	SDL_SetRenderDrawBlendMode(Renderer::renderer_ptr, SDL_BLENDMODE_BLEND);
	while (!ImageDB::pixImgQueue.empty()) {
		auto& pix = ImageDB::pixImgQueue.front();
		SDL_SetRenderDrawColor(Renderer::renderer_ptr, pix.r, pix.g, pix.b, pix.a);
		SDL_RenderDrawPoint(Renderer::renderer_ptr, pix.x, pix.y);

		ImageDB::pixImgQueue.pop();
	}
	SDL_SetRenderDrawBlendMode(Renderer::renderer_ptr, SDL_BLENDMODE_NONE);

	Helper::SDL_RenderPresent(Renderer::renderer_ptr);
}