/*****************************************************************//**
 * @file   input_manager.h
 * @brief  输入管理类
 * @version 2.0
 *
 * @author Shallowshades
 * @date   2026.03.17
 *********************************************************************/

#pragma once
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <string_view>
#include <unordered_map>
#include <vector>
#include <array>
#include <variant>
#include <functional>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_events.h>
#include <glm/vec2.hpp>
#include <entt/signal/sigh.hpp>
#include <entt/signal/fwd.hpp>

namespace engine::core {
	class Config;
}

namespace engine::input {
	/**
	 * @brief 动作状态类.
	 */
	enum class ActionState {
		PRESSED,			///< @brief 动作在本帧刚刚被按下
		HELD,				///< @brief 动作被持续按下
		RELEASED,			///< @brief 动作在本帧刚刚被释放
		INACTIVE,			///< @brief 动作未激活
	};

	/**
	 * @brief 输入管理类, 负责处理输入事件和动作状态.
	 *
	 * 该类管理输入事件, 将渐渐转换为动作状态, 并提供查询动作状态的功能
	 * 它还处理鼠标位置的逻辑坐标转换.
	 */
	class InputManager final {
	public:
		/**
		 * @brief 构造函数.
		 * @param renderer 指向SDL_Renderer的指针
		 * @param config 配置对象
		 * @throws 如果任一指针为nullptr抛出std::runtime_error
		 */
		InputManager(SDL_Renderer* renderer, const engine::core::Config* config, entt::dispatcher* dispatcher);

		/**
		 * @brief 注册一个动作的回调函数.
		 *
		 * @param actionNameId 动作名称哈希
		 * @param actionState 动作状态, 默认为按下瞬间
		 * @return 一个sink对象, 用于注册回调函数
		 */
		entt::sink<entt::sigh<bool()>> onAction(entt::id_type, ActionState actionState = ActionState::PRESSED);

		/**
		 * < @brief 更新输入状态, 每轮循环最先调用.
		 */
		void update();
		void quit();

		// 保留动作状态检查, 提供不同的使用选择
		bool isActionDown(entt::id_type actionName) const;														///< @brief 动作当前是否触发 (持续按下或本帧按下)
		bool isActionPressed(entt::id_type actionName) const;													///< @brief 动作是否在本帧刚刚按下
		bool isActionReleased(entt::id_type actionName) const;													///< @brief 动作是否在本帧刚刚释放

		glm::vec2 getMousePosition() const;																			///< @brief 获取鼠标位置(屏幕坐标)
		glm::vec2 getLogicalMousePosition() const;																	///< @brief 获取鼠标位置(逻辑坐标)

		// --- 触摸输入 ---
		bool isTouchDevice() const;																					///< @brief 当前是否为触摸设备
		bool isTouchActive() const;																					///< @brief 当前是否有触摸按下
		glm::vec2 getTouchPosition() const;																			///< @brief 获取触摸位置(逻辑坐标)
		glm::vec2 getTouchStartPosition() const;																	///< @brief 获取本次触摸开始位置(逻辑坐标)

		// --- UI 点击消费 ---
		void setUiHitTester(std::function<bool(const glm::vec2&)> tester);										///< @brief 设置UI命中测试回调, 用于标记UI消费点击
		bool isClickConsumed() const;																				///< @brief 当前点击是否已被UI消费
		void consumeNextClick();																					///< @brief 手动消费下一次点击
		void clearClickConsumed();																					///< @brief 清除点击消费标记

	private:
		void processEvent(const SDL_Event& event);																	///< @brief 处理SDL事件, 将按键转换为动作状态
		void handleFingerEvent(const SDL_Event& event);																///< @brief 处理触摸FINGER事件, 映射为鼠标/触摸动作
		void initializeMappings(const engine::core::Config* config);												///< @brief 根据Config配置初始化映射表

		void updateActionState(entt::id_type, bool isInputActive, bool isRepeatEvent);								///< @brief 辅助更新动作状态
		SDL_Scancode scancodeFromString(std::string_view keyName);													///< @brief 将字符串键名转换为SDL_Scancode
		Uint32 mouseButtonFromString(std::string_view buttonName);													///< @brief 将字符串按钮名字转换为SDL_Button
	private:
		static constexpr std::string_view mLogTag = "InputManager";

		entt::dispatcher* mDispatcher;																				///< @brief 事件分发器
		SDL_Renderer* mSDLRenderer;																					///< @brief 用于获取逻辑坐标的SDL_Renderer指针
		std::unordered_map<entt::id_type, std::array<entt::sigh<bool()>, 3>> mActionsToFunc;						///< @brief 存储动作名称函数列表的映射
		std::unordered_map<std::variant<SDL_Scancode, Uint32>, std::vector<entt::id_type>> mInputToActions;			///< @brief 从键盘(Scancode)到关联的动作名称列表
		std::unordered_map<entt::id_type, ActionState> mActionStates;												///< @brief 存储每个动作的当前状态
		bool mShouldQuit = false;																					///< @brief 推出标志
		glm::vec2 mMousePosition;																					///< @brief 鼠标位置(针对屏幕坐标)
		glm::vec2 mLogicalMousePosition;																			///< @brief 鼠标位置(针对逻辑坐标)

		// --- 触摸状态 ---
		bool mIsTouchDevice = false;																				///< @brief 是否为触摸设备
		bool mIsTouchActive = false;																				///< @brief 当前是否有触摸按下
		SDL_FingerID mActiveFingerId = 0;																			///< @brief 当前活动手指ID
		glm::vec2 mTouchPosition;																					///< @brief 触摸位置(逻辑坐标)
		glm::vec2 mTouchStartPosition;																				///< @brief 本次触摸开始位置(逻辑坐标)

		// --- UI 点击消费 ---
		std::function<bool(const glm::vec2&)> mUiHitTester;															///< @brief UI命中测试回调
		bool mClickConsumed = false;																				///< @brief 当前帧/事件点击是否已被UI消费
	};
} // engine::input

#endif // !INPUT_MANAGER_H
