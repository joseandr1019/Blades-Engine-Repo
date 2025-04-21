#ifndef INPUT_H
#define INPUT_H

#include <string>
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"
#include "SDL2/SDL.h"

enum INPUT_STATE { INPUT_STATE_UP, INPUT_STATE_JUST_BECAME_DOWN, INPUT_STATE_DOWN, INPUT_STATE_JUST_BECAME_UP };

class Input
{
public:
	static void Init(); // Call before main loop begins.
	static void ProcessEvent(const SDL_Event& e); // Call every frame at start of event loop.
	static void LateUpdate();

	static bool GetKey(const std::string& key);
	static bool GetKeyDown(const std::string& key);
	static bool GetKeyUp(const std::string& key);

	static glm::vec2 GetMousePosition() {
		return mousePos;
	}
	static bool GetMouseButton(int button_num);
	static bool GetMouseButtonDown(int button_num);
	static bool GetMouseButtonUp(int button_num);
	static float GetMouseScrollDelta() {
		return mouseScroll;
	}
	static void HideCursor() {
		SDL_ShowCursor(SDL_DISABLE);
	}
	static void ShowCursor() {
		SDL_ShowCursor(SDL_ENABLE);
	}

	static void LuaInit();
	static inline SDL_Scancode StringKeycode(const std::string& stringKey);
private:
	static inline std::unordered_map<std::string, SDL_Scancode> string_keycodes = std::unordered_map<std::string, SDL_Scancode>{
		// Directional (arrow) Keys
		{"up", SDL_SCANCODE_UP},
		{"down", SDL_SCANCODE_DOWN},
		{"right", SDL_SCANCODE_RIGHT},
		{"left", SDL_SCANCODE_LEFT},

		// Misc Keys
		{"escape", SDL_SCANCODE_ESCAPE},

		// Modifier Keys
		{"lshift", SDL_SCANCODE_LSHIFT},
		{"rshift", SDL_SCANCODE_RSHIFT},
		{"lctrl", SDL_SCANCODE_LCTRL},
		{"rctrl", SDL_SCANCODE_RCTRL},
		{"lalt", SDL_SCANCODE_LALT},
		{"ralt", SDL_SCANCODE_RALT},

		// Editing Keys
		{"tab", SDL_SCANCODE_TAB},
		{"return", SDL_SCANCODE_RETURN},
		{"enter", SDL_SCANCODE_RETURN},
		{"backspace", SDL_SCANCODE_BACKSPACE},
		{"delete", SDL_SCANCODE_DELETE},
		{"insert", SDL_SCANCODE_INSERT},

		// Character Keys
		{"space", SDL_SCANCODE_SPACE},
		{"a", SDL_SCANCODE_A},
		{"b", SDL_SCANCODE_B},
		{"c", SDL_SCANCODE_C},
		{"d", SDL_SCANCODE_D},
		{"e", SDL_SCANCODE_E},
		{"f", SDL_SCANCODE_F},
		{"g", SDL_SCANCODE_G},
		{"h", SDL_SCANCODE_H},
		{"i", SDL_SCANCODE_I},
		{"j", SDL_SCANCODE_J},
		{"k", SDL_SCANCODE_K},
		{"l", SDL_SCANCODE_L},
		{"m", SDL_SCANCODE_M},
		{"n", SDL_SCANCODE_N},
		{"o", SDL_SCANCODE_O},
		{"p", SDL_SCANCODE_P},
		{"q", SDL_SCANCODE_Q},
		{"r", SDL_SCANCODE_R},
		{"s", SDL_SCANCODE_S},
		{"t", SDL_SCANCODE_T},
		{"u", SDL_SCANCODE_U},
		{"v", SDL_SCANCODE_V},
		{"w", SDL_SCANCODE_W},
		{"x", SDL_SCANCODE_X},
		{"y", SDL_SCANCODE_Y},
		{"z", SDL_SCANCODE_Z},
		{"0", SDL_SCANCODE_0},
		{"1", SDL_SCANCODE_1},
		{"2", SDL_SCANCODE_2},
		{"3", SDL_SCANCODE_3},
		{"4", SDL_SCANCODE_4},
		{"5", SDL_SCANCODE_5},
		{"6", SDL_SCANCODE_6},
		{"7", SDL_SCANCODE_7},
		{"8", SDL_SCANCODE_8},
		{"9", SDL_SCANCODE_9},
		{"/", SDL_SCANCODE_SLASH},
		{";", SDL_SCANCODE_SEMICOLON},
		{"=", SDL_SCANCODE_EQUALS},
		{"-", SDL_SCANCODE_MINUS},
		{".", SDL_SCANCODE_PERIOD},
		{",", SDL_SCANCODE_COMMA},
		{"[", SDL_SCANCODE_LEFTBRACKET},
		{"]", SDL_SCANCODE_RIGHTBRACKET},
		{"\\", SDL_SCANCODE_BACKSLASH},
		{"'", SDL_SCANCODE_APOSTROPHE}
	};

	static inline std::vector<INPUT_STATE> keyboard_states = std::vector<INPUT_STATE>(SDL_NUM_SCANCODES, INPUT_STATE::INPUT_STATE_UP);
	static inline std::vector<SDL_Scancode> just_became_down_scancodes;
	static inline std::vector<SDL_Scancode> just_became_up_scancodes;

	static inline glm::vec2 mousePos = glm::vec2(0.0f, 0.0f);
	static inline std::vector<INPUT_STATE> mouseButton_states = std::vector<INPUT_STATE>(3, INPUT_STATE::INPUT_STATE_UP);
	static inline std::vector<int> just_became_down_buttons;
	static inline std::vector<int> just_became_up_buttons;
	static inline float mouseScroll = float(0.0f);

	Input() {};
};

#endif