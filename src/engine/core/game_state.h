/*****************************************************************//**
 * @file   game_state.h
 * @brief  游戏状态类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.01.21
 *********************************************************************/

#pragma once
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <optional>
#include <string>
#include <glm/vec2.hpp>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>

namespace engine::core {

/**
* @enum State
* @brief 定义游戏可能处于的宏观状态。
*/
enum class State {
    Title,          ///< @brief 标题界面
    Playing,        ///< @brief 正常游戏进行中
    Paused,         ///< @brief 游戏暂停（通常覆盖菜单界面）
    GameOver,       ///< @brief 游戏结束界面
    // 可以根据需要添加更多状态，如 Cutscene, SettingsMenu 等
};

/**
* @brief 管理和查询游戏的全局宏观状态。
*
* 提供一个中心点来确定游戏当前处于哪个主要模式，
* 以便其他系统（输入、渲染、更新等）可以相应地调整其行为。
*/
class GameState final {
public:
    /**
	* @brief 构造函数，初始化游戏状态。
	* @param window SDL窗口，必须传入有效值。
	* @param renderer SDL渲染器，必须传入有效值。
	* @param initial_state 游戏的初始状态，默认为 Title
	*/
    explicit GameState(SDL_Window* window, SDL_Renderer* renderer, State initial_state = State::Title);

    State getCurrentState() const { return mCurrentState; }
    void setState(State new_state);
    glm::vec2 getWindowSize() const;
    void setWindowSize(glm::vec2 new_size);
    glm::vec2 getLogicalSize() const;
    void setLogicalSize(glm::vec2 new_size);

    // --- 便捷查询方法 ---
    bool isInTitle() const { return mCurrentState == State::Title; }
    bool isPlaying() const { return mCurrentState == State::Playing; }
    bool isPaused() const { return mCurrentState == State::Paused; }
    bool isGameOver() const { return mCurrentState == State::GameOver; }

private:
	SDL_Window* mWindow = nullptr;              ///< @brief SDL窗口，用于获取窗口大小
	SDL_Renderer* mRenderer = nullptr;          ///< @brief SDL渲染器，用于获取逻辑分辨率
	State mCurrentState = State::Title;        ///< @brief 当前游戏状态
};

} // namespace engine::core

#endif // !GAME_STATE_H
