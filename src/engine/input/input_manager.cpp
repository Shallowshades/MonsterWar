#include "input_manager.h"
#include "../core/config.h"
#include "../utils/events.h"
#include <stdexcept>
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

namespace engine::input {
	InputManager::InputManager(SDL_Renderer* renderer, const engine::core::Config* config, entt::dispatcher* dispatcher)
		: mSDLRenderer(renderer), mDispatcher(dispatcher)
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

	entt::sink<entt::sigh<bool()>> InputManager::onAction(entt::id_type action_name_id, ActionState actionState) {
		// 如果actionName不存在, 自动创建一个std::array
		// array.at() 会进行边界检查, 更安全
		return mActionsToFunc[action_name_id].at(static_cast<size_t>(actionState));
	}

	void InputManager::update() {
		// 1.根据上一帧的值更新默认的动作状态
		for (auto& [actionNameId, state] : mActionStates) {
			if (state == ActionState::PRESSED) {
				state = ActionState::HELD;
			}
			else if (state == ActionState::RELEASED) {
				state = ActionState::INACTIVE;
			}
		}

		// 2.处理所有待处理的SDL事件(设定ActionStates的值)
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);    // ImGui 步骤2 处理 ImGui 事件
			processEvent(event);
		}

		// 3.触发回调
		for (auto& [actionNameId, state] : mActionStates) {
			if (state != ActionState::INACTIVE) {
				// 且有绑定函数
				if (auto it = mActionsToFunc.find(actionNameId); it != mActionsToFunc.end()) {
					// collect 方法可以获取回调函数返回值, 放入lambda函数的参数中
					// 而lambda函数的返回值为真, 停止分发信号
					// 分发信号的顺序为后绑定先调用
					it->second.at(static_cast<size_t>(state)).collect([](bool result) {
						return result;
						});
				}
			}
		}
	}

	void InputManager::quit() {
		mDispatcher->trigger<engine::utils::QuitEvent>();
	}

	bool InputManager::isActionDown(entt::id_type action_name_id) const {
		if (auto iter = mActionStates.find(action_name_id); iter != mActionStates.end()) {
			return iter->second == ActionState::PRESSED ||
				iter->second == ActionState::HELD;
		}
		return false;
	}

	bool InputManager::isActionPressed(entt::id_type action_name_id) const {
		if (auto iter = mActionStates.find(action_name_id); iter != mActionStates.end()) {
			return iter->second == ActionState::PRESSED;
		}
		return false;
	}

	bool InputManager::isActionReleased(entt::id_type action_name_id) const {
		if (auto iter = mActionStates.find(action_name_id); iter != mActionStates.end()) {
			return iter->second == ActionState::RELEASED;
		}
		return false;
	}

	glm::vec2 InputManager::getMousePosition() const {
		return mMousePosition;
	}

	glm::vec2 InputManager::getLogicalMousePosition() const {
		return mLogicalMousePosition;
	}

	void InputManager::processEvent(const SDL_Event& event) {
		// 如果 ImGui 捕获了鼠标，则不处理该事件(避免穿透到游戏中)
		if (ImGui::GetIO().WantCaptureMouse) {
			return;
		}

		switch (event.type) {
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP: {
			// 获取按键的scancode
			SDL_Scancode scancode = event.key.scancode;
			bool isDown = event.key.down;
			bool isRepeat = event.key.repeat;

			auto iter = mInputToActions.find(scancode);
			if (iter != mInputToActions.end()) {
				const std::vector<entt::id_type>& associatedActions = iter->second;
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
			auto iter = mInputToActions.find(button);
			if (iter != mInputToActions.end()) {
				const std::vector<entt::id_type>& associatedActions = iter->second;
				for (const auto& actionName : associatedActions) {
					// 鼠标事件不考虑repeat, 所以第三个参数传false
					updateActionState(actionName, isDown, false);
				}
			}
			// 在点击时更新鼠标位置, 同时更新逻辑位置, 避免多次一帧内多次调用的性能损耗
			mMousePosition = { event.button.x, event.button.y };
			SDL_RenderCoordinatesFromWindow(mSDLRenderer, mMousePosition.x, mMousePosition.y, &mLogicalMousePosition.x, &mLogicalMousePosition.y);
			break;
		}
		case SDL_EVENT_MOUSE_MOTION:
			mMousePosition = { event.motion.x, event.motion.y };
			SDL_RenderCoordinatesFromWindow(mSDLRenderer, mMousePosition.x, mMousePosition.y, &mLogicalMousePosition.x, &mLogicalMousePosition.y);
			break;
		case SDL_EVENT_QUIT:
			mShouldQuit = true;
			quit();
			break;
		default:
			break;
		}
	}

	void InputManager::initializeMappings(const engine::core::Config* config) {
		spdlog::trace("{} 初始化输入映射", mLogTag.data());
		if (!config) {
			spdlog::error("{} 输入管理器: Config 为空指针", mLogTag.data());
			throw std::runtime_error(mLogTag.data() + std::string("Config 为空指针"));
		}
		auto actionsToKeyname = config->mInputMappings;
		mInputToActions.clear();
		mActionStates.clear();

		// 如果配置中没有定义鼠标按钮动作(通常不需要配置), 则添加默认映射, 用于UI
		// NOTE: 动作名必须与游戏实际订阅的 mouse_left/mouse_right 完全一致（Button状态机/SelectionSystem/PlaceUnitSystem）。
		// 配置缺失时（如 wasm 首次运行 assets/save/config.json 不存在，回退到默认映射），
		// 若此处仍用旧的 MouseLeftClick 命名，则 hashed id 不一致 → mouse_left 信号永不触发 → 鼠标点击完全失效。
		if (actionsToKeyname.find("mouse_left") == actionsToKeyname.end()) {
			spdlog::debug("{} 配置中没有定义 'mouse_left' 动作, 添加默认映射到 'MouseLeft'.", mLogTag.data());
			actionsToKeyname["mouse_left"] = { "MouseLeft" };
		}
		if (actionsToKeyname.find("mouse_right") == actionsToKeyname.end()) {
			spdlog::debug("{} 配置中没有定义 'mouse_right' 动作, 添加默认映射到 'MouseRight'.", mLogTag.data());
			actionsToKeyname["mouse_right"] = { "MouseRight" };
		}

		// 遍历 动作 -> 按键名称 的映射
		for (const auto& [actionName, keyNames] : actionsToKeyname) {
			// 每个动作对应一个动作状态, 初始化INACTIVE
			auto actionNameId = entt::hashed_string(actionName.c_str());
			mActionStates[actionNameId] = ActionState::INACTIVE;
			spdlog::trace("{} 映射动作: {}", mLogTag.data(), actionName);
			// 设置 "按键 -> 动作" 的映射
			for (const auto& keyName : keyNames) {
				spdlog::trace("{} 当前按键名称 '{}'", mLogTag.data(), keyName);
				SDL_Scancode scancode = scancodeFromString(keyName);
				Uint32 mouseButton = mouseButtonFromString(keyName);
				// TODO: 未来可添加其他输入类型 ......

				if (scancode != SDL_SCANCODE_UNKNOWN) {
					mInputToActions[scancode].push_back(actionNameId);
					spdlog::trace("{} 按键映射: {} (Scancode: {} 到动作: {})", mLogTag.data(), keyName, static_cast<int>(scancode), actionName);
				}
				else if (mouseButton != 0) {	// 如果鼠标按钮有效, 则将action添加到mMouseActionMappings中
					mInputToActions[mouseButton].push_back(actionNameId);
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

	void InputManager::updateActionState(entt::id_type actionName, bool isInputActive, bool isRepeatEvent) {
		auto iter = mActionStates.find(actionName);
		if (iter == mActionStates.end()) {
			spdlog::warn("{} 尝试更新未注册的动作状态: {}", mLogTag.data(), actionName);
			return;
		}

		if (isInputActive) {
			if (isRepeatEvent) {
				iter->second = ActionState::HELD;
			}
			else {
				iter->second = ActionState::PRESSED;
			}
		}
		else {
			iter->second = ActionState::RELEASED;
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
