#include "ComponentDB.h"
#include "EngineUtils.h"
#include "InputManager.h"
#include "SceneDB.h"

#include <stdexcept>
#include <unordered_map>

#include "glm/glm.hpp"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "SDL2/SDL.h"

void Input::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Input")
		.addFunction("GetKey", &Input::GetKey)
		.addFunction("GetKeyDown", &Input::GetKeyDown)
		.addFunction("GetKeyUp", &Input::GetKeyUp)
		.addFunction("GetMouseButton", &Input::GetMouseButton)
		.addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
		.addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
		.addFunction("GetMousePosition", &Input::GetMousePosition)
		.addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
		.addFunction("ShowCursor", &Input::ShowCursor)
		.addFunction("HideCursor", &Input::HideCursor)
		.endNamespace();
}

SDL_Scancode Input::StringKeycode(const std::string& stringKey) {
	auto it = Input::string_keycodes.find(stringKey);
	if (it != Input::string_keycodes.end()) {
		return it->second;
	}
	else {
		return SDL_SCANCODE_UNKNOWN;
	}
}

void Input::ProcessEvent(const SDL_Event& e) {
	if (e.type == SDL_KEYDOWN) {
		if (Input::keyboard_states[e.key.keysym.scancode] == INPUT_STATE::INPUT_STATE_UP
			|| Input::keyboard_states[e.key.keysym.scancode] == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP) {
			Input::keyboard_states[e.key.keysym.scancode] = INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN;
			Input::just_became_down_scancodes.push_back(e.key.keysym.scancode);
		}
	}

	else if (e.type == SDL_KEYUP) {
		if (Input::keyboard_states[e.key.keysym.scancode] == INPUT_STATE::INPUT_STATE_DOWN
			|| Input::keyboard_states[e.key.keysym.scancode] == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN) {
			Input::keyboard_states[e.key.keysym.scancode] = INPUT_STATE::INPUT_STATE_JUST_BECAME_UP;
			Input::just_became_up_scancodes.push_back(e.key.keysym.scancode);
		}
	}

	else if (e.type == SDL_MOUSEMOTION) {
		Input::mousePos.x = e.motion.x;
		Input::mousePos.y = e.motion.y;
	}

	else if (e.type == SDL_MOUSEBUTTONDOWN) {
		if (Input::mouseButton_states[e.button.button - 1] == INPUT_STATE::INPUT_STATE_UP
			|| Input::mouseButton_states[e.button.button - 1] == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP) {
			Input::mouseButton_states[e.button.button - 1] = INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN;
			Input::just_became_down_buttons.push_back(e.button.button - 1);
		}
	}

	else if (e.type == SDL_MOUSEBUTTONUP) {
		if (Input::mouseButton_states[e.button.button - 1] == INPUT_STATE::INPUT_STATE_DOWN
			|| Input::mouseButton_states[e.button.button - 1] == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN) {
			Input::mouseButton_states[e.button.button - 1] = INPUT_STATE::INPUT_STATE_JUST_BECAME_UP;
			Input::just_became_up_buttons.push_back(e.button.button - 1);
		}
	}

	else if (e.type == SDL_MOUSEWHEEL) {
		mouseScroll = e.wheel.preciseY;
	}
}

void Input::LateUpdate() {
	for (auto& scancode : just_became_down_scancodes) {
		if (Input::keyboard_states[scancode] == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN) {
			Input::keyboard_states[scancode] = INPUT_STATE::INPUT_STATE_DOWN;
		}
	}
	Input::just_became_down_scancodes.clear();

	for (auto& scancode : just_became_up_scancodes) {
		if (Input::keyboard_states[scancode] == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP) {
			Input::keyboard_states[scancode] = INPUT_STATE::INPUT_STATE_UP;
		}
	}
	Input::just_became_up_scancodes.clear();

	for (auto& button : just_became_down_buttons) {
		if (Input::mouseButton_states[button] == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN) {
			Input::mouseButton_states[button] = INPUT_STATE::INPUT_STATE_DOWN;
		}
	}
	Input::just_became_down_buttons.clear();

	for (auto& button : just_became_up_buttons) {
		if (Input::mouseButton_states[button] == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP) {
			Input::mouseButton_states[button] = INPUT_STATE::INPUT_STATE_UP;
		}
	}
	Input::just_became_up_buttons.clear();

	Input::mouseScroll = 0.0f;
}

bool Input::GetKey(const std::string& key) {
	SDL_Scancode keycode = Input::StringKeycode(key);
	INPUT_STATE state = Input::keyboard_states[keycode];
	return (state == INPUT_STATE::INPUT_STATE_DOWN
		|| state == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN);
}

bool Input::GetKeyDown(const std::string& key) {
	SDL_Scancode keycode = Input::StringKeycode(key);
	INPUT_STATE state = Input::keyboard_states[keycode];
	return (state == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN);
}

bool Input::GetKeyUp(const std::string& key) {
	SDL_Scancode keycode = Input::StringKeycode(key);
	INPUT_STATE state = Input::keyboard_states[keycode];
	return (state == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP);
}

bool Input::GetMouseButton(int button_num) {
	if (button_num > 3 || button_num < 1) {
		return false;
	}
	else {
		INPUT_STATE state = Input::mouseButton_states[button_num - 1];
		return (state == INPUT_STATE::INPUT_STATE_DOWN
			|| state == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN);
	}
}

bool Input::GetMouseButtonDown(int button_num) {
	if (button_num > 3 || button_num < 1) {
		return false;
	}
	else {
		INPUT_STATE state = Input::mouseButton_states[button_num - 1];
		return (state == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN);
	}
}

bool Input::GetMouseButtonUp(int button_num) {
	if (button_num > 3 || button_num < 1) {
		return false;
	}
	else {
		INPUT_STATE state = Input::mouseButton_states[button_num - 1];
		return (state == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP);
	}
}