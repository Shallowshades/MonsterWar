#include "input_manager.h"
#include "../core/config.h"
#include <stdexcept>
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>

namespace engine::input {
InputManager::InputManager(SDL_Renderer* renderer, const engine::core::Config* config)
	: mSDLRenderer(renderer)
{
	if (!mSDLRenderer) {
		spdlog::error("{} 输入管理器: SDL_Renderer为空指针.", mLogTag.data());
		throw std::runtime_error(mLogTag.data() + std::string("输入管理器: SDL_Renderer为空指针"));
	}
	initializeMappings(config);

	// 获取初始鼠标位置
	float x, y;
	SDL_GetMouseState(&x, &y);
	mMousePosition = { x, y };
	spdlog::trace("{} 初始鼠标位置: ({}, {})", mLogTag.data(), mMousePosition.x, mMousePosition.y);
}

void InputManager::update() {
	// 1.根据上一帧的值更新默认的动作状态
	for (auto& [actionName, state] : mActionStates) {
		if (state == ActionState::PRESSED_THIS_FRAME) {
			state = ActionState::HELD_DOWN;
		}
		else if (state == ActionState::RELEASED_THIS_FRAME) {
			state = ActionState::INACTIVE;
		}
	}

	// 2.处理所有待处理的SDL事件(设定ActionStates的值)
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		processEvent(event);
	}
}

bool InputManager::isActionDown(std::string_view actionName) const {
	if (auto iter = mActionStates.find(std::string(actionName)); iter != mActionStates.end()) {
		return iter->second == ActionState::PRESSED_THIS_FRAME ||
			iter->second == ActionState::HELD_DOWN;
	}
	return false;
}

bool InputManager::isActionPressed(std::string_view actionName) const {
	if (auto iter = mActionStates.find(std::string(actionName)); iter != mActionStates.end()) {
		return iter->second == ActionState::PRESSED_THIS_FRAME;
	}
	return false;
}

bool InputManager::isActionReleased(std::string_view actionName) const {
	if (auto iter = mActionStates.find(std::string(actionName)); iter != mActionStates.end()) {
		return iter->second == ActionState::RELEASED_THIS_FRAME;
	}
	return false;
}

bool InputManager::shouldQuit() const {
	return mShouldQuit;
}

void InputManager::setShouldQuit(bool shouldQuit) {
	mShouldQuit = shouldQuit;
}

glm::vec2 InputManager::getMousePosition() const {
	return mMousePosition;
}

glm::vec2 InputManager::getLogicalMousePosition() const {
	glm::vec2 logicPosition;
	SDL_RenderCoordinatesFromWindow(mSDLRenderer, mMousePosition.x, mMousePosition.y, &logicPosition.x, &logicPosition.y);
	return logicPosition;
}

void InputManager::processEvent(const SDL_Event& event) {
	switch (event.type) {
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP: {
		// 获取按键的scancode
		SDL_Scancode scancode = event.key.scancode;
		bool isDown = event.key.down;
		bool isRepeat = event.key.repeat;

		auto iter = mInputToActionsMappings.find(scancode);
		if (iter != mInputToActionsMappings.end()) {
			const std::vector<std::string>& associatedActions = iter->second;
			for (const auto& actionName : associatedActions) {
				updateActionState(actionName, isDown, isRepeat);
			}
		}
		break;
	}
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP: {
		Uint32 button = event.button.button;
		bool isDown = event.button.down;
		auto iter = mInputToActionsMappings.find(button);
		if (iter != mInputToActionsMappings.end()) {
			const std::vector<std::string>& associatedActions = iter->second;
			for (const auto& actionName : associatedActions) {
				// 鼠标事件不考虑repeat, 所以第三个参数传false
				updateActionState(actionName, isDown, false);
			}
		}
		// 在点击时更新鼠标位置
		mMousePosition = { event.button.x, event.button.y };
		break;
	}
	case SDL_EVENT_MOUSE_MOTION:
		mMousePosition = { event.motion.x, event.motion.y };
		break;
	case SDL_EVENT_QUIT:
		mShouldQuit = true;
		break;
	}
}

void InputManager::initializeMappings(const engine::core::Config* config) {
	spdlog::trace("{} 初始化输入映射", mLogTag.data());
	if (!config) {
		spdlog::error("{} 输入管理器: Config 为空指针", mLogTag.data());
		throw std::runtime_error(mLogTag.data() + std::string("Config 为空指针"));
	}
	mActionsToKeyNameMappings = config->mInputMappings;
	mInputToActionsMappings.clear();
	mActionStates.clear();

	// 如果配置中没有定义鼠标按钮动作(通常不需要配置), 则添加默认映射, 用于UI
	if (mActionsToKeyNameMappings.find("MouseLeftClick") == mActionsToKeyNameMappings.end()) {
		spdlog::debug("{} 配置中没有定义 'MouseLeftClick' 动作, 添加默认映射到 'MouseLeft'.", mLogTag.data());
		mActionsToKeyNameMappings["MouseLeftClick"] = { "MouseLeft" };
	}
	if (mActionsToKeyNameMappings.find("MouseRightClick") == mActionsToKeyNameMappings.end()) {
		spdlog::debug("{} 配置中没有定义 'MouseRightClick' 动作, 添加默认映射到 'MouseRight'.", mLogTag.data());
		mActionsToKeyNameMappings["MouseRightClick"] = { "MouseRight" };
	}

	// 遍历 动作 -> 按键名称 的映射
	for (const auto& [actionName, keyNames] : mActionsToKeyNameMappings) {
		// 每个动作对应一个动作状态, 初始化INACTIVE
		mActionStates[actionName] = ActionState::INACTIVE;
		spdlog::trace("{} 映射动作: {}", mLogTag.data(), actionName);
		// 设置 "按键 -> 动作" 的映射
		for (const auto& keyName : keyNames) {
			spdlog::trace("{} 当前按键名称 '{}'", mLogTag.data(), keyName);
			SDL_Scancode scancode = scancodeFromString(keyName);
			Uint32 mouseButton = mouseButtonFromString(keyName);
			// TODO: 未来可添加其他输入类型 ......

			if (scancode != SDL_SCANCODE_UNKNOWN) {
				mInputToActionsMappings[scancode].push_back(actionName);
				spdlog::trace("{} 按键映射: {} (Scancode: {} 到动作: {})", mLogTag.data(), keyName, static_cast<int>(scancode), actionName);
			}
			else if (mouseButton != 0) {	// 如果鼠标按钮有效, 则将action添加到mMouseActionMappings中
				mInputToActionsMappings[mouseButton].push_back(actionName);
				spdlog::trace("{} 鼠标映射: {} (Button ID: {} 到动作: {})", mLogTag.data(), keyName, static_cast<int>(mouseButton), actionName);
			}
			// TODO: more input type
			else {
				spdlog::warn("{} 输入映射警告: 未知键或按钮名称 '{}' 用于动作 '{}'", mLogTag.data(), keyName, actionName);
			}
		}
	}
	spdlog::trace("{} 输入映射初始化成功", mLogTag.data());
}

void InputManager::updateActionState(std::string_view actionName, bool isInputActive, bool isRepeatEvent) {
	auto iter = mActionStates.find(std::string(actionName));
	if (iter == mActionStates.end()) {
		spdlog::warn("{} 尝试更新未注册的动作状态: {}", mLogTag.data(), actionName);
		return;
	}

	if (isInputActive) {
		if (isRepeatEvent) {
			iter->second = ActionState::HELD_DOWN;
		}
		else {
			iter->second = ActionState::PRESSED_THIS_FRAME;
		}
	}
	else {
		iter->second = ActionState::RELEASED_THIS_FRAME;
	}
}

SDL_Scancode InputManager::scancodeFromString(std::string_view keyName) {
	return SDL_GetScancodeFromName(keyName.data());
}

Uint32 InputManager::mouseButtonFromString(std::string_view buttonName) {
	if (buttonName == "MouseLeft") return SDL_BUTTON_LEFT;
	if (buttonName == "MouseMiddle") return SDL_BUTTON_MIDDLE;
	if (buttonName == "MouseRight") return SDL_BUTTON_RIGHT;

	// SDL还定义了SDL_BUTTON_X1和SDL_BUTTON_X2
	if (buttonName == "MouseX1") return SDL_BUTTON_X1;
	if (buttonName == "MouseX2") return SDL_BUTTON_X2;
	return 0;	// 0表示无效值
}
} // namespace engine::input
