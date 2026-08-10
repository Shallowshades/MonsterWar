/*****************************************************************//**
 * @file   ui_interactive.h
 * @brief  可交互元素的基类, 继承自UIElement
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.03.18
 *********************************************************************/

#pragma once
#ifndef UI_INTERACTIVE_H
#define UI_INTERACTIVE_H

#include "ui_element.h"
#include "state/ui_state.h"
#include "../render/image.h"   // 需要引入头文件而不是前置声明（map容器创建时可能会检查内部元素是否有析构定义）
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <entt/core/hashed_string.hpp>

namespace engine::core {
    class Context;
}

namespace engine::ui {

/**
* @brief 可交互UI元素的基类,继承自UIElement
*
* 定义了可交互UI元素的通用属性和行为。
* 管理UI状态的切换和交互逻辑。
* 提供更新和渲染的虚方法。
* @note 输入不再由状态机轮询，而是通过 InputManager 的信号回调驱动（状态订阅 mouse_left 信号）。
*       状态切换通过 setNextState 延迟到下一帧 update 应用，避免在信号分发中直接销毁状态对象。
*/
class UIInteractive : public UIElement {
public:
    UIInteractive(engine::core::Context& context, const glm::vec2& position = { 0.0f, 0.0f }, const glm::vec2& size = { 0.0f, 0.0f });
    ~UIInteractive() override;

    virtual void clicked() {}                                                               ///< @brief 点击回调（子类重写）
    virtual void hover_enter() {}                                                           ///< @brief 悬停进入回调（子类重写）
    virtual void hover_leave() {}                                                           ///< @brief 悬停离开回调（子类重写）

    void addImage(entt::id_type nameId, engine::render::Image image);                       ///< @brief 添加图片
    void setCurrentImage(entt::id_type nameId);                                             ///< @brief 设置当前显示的精灵
    void addSound(entt::id_type nameId, std::string_view filePath);                         ///< @brief 添加音效
    void playSound(entt::id_type nameId);                                                   ///< @brief 播放音效
    void setNextState(std::unique_ptr<engine::ui::state::UIState> state);                   ///< @brief 延迟设置下一个状态（下一帧update应用）
    // --- Getters and Setters ---
    void setState(std::unique_ptr<engine::ui::state::UIState> state);                       ///< @brief 设置当前状态(立即应用并调用enter)
    engine::ui::state::UIState* getState() const { return mState.get(); }                   ///< @brief 获取当前状态
    engine::core::Context& getContext() const { return mContext; }                          ///< @brief 获取引擎上下文

    void setInteractive(bool interactive) { mInteractive = interactive; }                   ///< @brief 设置是否可交互
    bool isInteractive() const { return mInteractive; }                                     ///< @brief 获取是否可交互

    // --- 核心方法 ---
    void update(float deltaTime, engine::core::Context& context) override;
    void render(engine::core::Context& context) override;

protected:
	engine::core::Context& mContext;                                                        ///< @brief 可交互元素很可能需要其他引擎组件
	std::unique_ptr<engine::ui::state::UIState> mState;                                     ///< @brief 当前状态
	std::unique_ptr<engine::ui::state::UIState> mNextState;                                 ///< @brief 延迟应用的下一个状态(事件回调中排队)
	std::unordered_map<entt::id_type, engine::render::Image> mImages;                       ///< @brief 精灵集合
	std::unordered_map<entt::id_type, entt::id_type> mSounds;                               ///< @brief 音效集合，key为音效名称ID，value为音效ID
	entt::id_type mCurrentImageId;                                                          ///< @brief 当前显示的精灵ID
	bool mInteractive = true;                                                               ///< @brief 是否可交互
};

} // namespace engine::ui

#endif // !UI_INTERACTIVE_H
