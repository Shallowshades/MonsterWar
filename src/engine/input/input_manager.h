/*****************************************************************//**
 * @file   input_manager.h
 * @brief  输入管理类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.10.21
 *********************************************************************/

#pragma once
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <variant>
#include <SDL3/SDL_render.h>
#include <glm/vec2.hpp>

namespace engine::core {
class Config;
}

namespace engine::input {
/**
 * @brief 动作状态类.
 */
enum class ActionState {
	INACTIVE,				///< @brief 动作未激活
	PRESSED_THIS_FRAME,		///< @brief 动作在本帧刚刚被按下
	HELD_DOWN,				///< @brief 动作被持续按下
	RELEASED_THIS_FRAME		///< @brief 动作在本帧刚刚被释放
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
	InputManager(SDL_Renderer* renderer, const engine::core::Config* config);

	void update();																								///< @brief 更新输入状态, 每轮循环最先调用

	bool isActionDown(std::string_view actionName) const;														///< @brief 动作当前是否触发 (持续按下或本帧按下)
	bool isActionPressed(std::string_view actionName) const;													///< @brief 动作是否在本帧刚刚按下
	bool isActionReleased(std::string_view actionName) const;													///< @brief 动作是否在本帧刚刚释放
	
	bool shouldQuit() const;																					///< @brief 查询退出状态

	void setShouldQuit(bool shouldQuit);																		///< @brief 设置退出状态

	glm::vec2 getMousePosition() const;																			///< @brief 获取鼠标位置(屏幕坐标)
	glm::vec2 getLogicalMousePosition() const;																	///< @brief 获取鼠标位置(逻辑坐标)

private:
	void processEvent(const SDL_Event& event);																	///< @brief 处理SDL事件, 将按键转换为动作状态
	void initializeMappings(const engine::core::Config* config);												///< @brief 根据Config配置初始化映射表

	void updateActionState(std::string_view, bool isInputActive, bool isRepeatEvent);							///< @brief 辅助更新动作状态
	SDL_Scancode scancodeFromString(std::string_view keyName);													///< @brief 将字符串键名转换为SDL_Scancode
	Uint32 mouseButtonFromString(std::string_view buttonName);													///< @brief 将字符串按钮名字转换为SDL_Button
private:
	static constexpr std::string_view mLogTag = "InputManager";

	SDL_Renderer* mSDLRenderer;																					///< @brief 用于获取逻辑坐标的SDL_Renderer指针
	std::unordered_map<std::string, std::vector<std::string>> mActionsToKeyNameMappings;						///< @brief 存储动作名称到按键名称列表的映射
	std::unordered_map<std::variant<SDL_Scancode, Uint32>, std::vector<std::string>> mInputToActionsMappings;	///< @brief 从键盘(Scancode)到关联的动作名称列表
	std::unordered_map<std::string, ActionState> mActionStates;													///< @brief 存储每个动作的当前状态
	bool mShouldQuit = false;																					///< @brief 推出标志
	glm::vec2 mMousePosition;																					///< @brief 鼠标位置(针对屏幕坐标)
};
} // engine::input

#endif // !INPUT_MANAGER_H
