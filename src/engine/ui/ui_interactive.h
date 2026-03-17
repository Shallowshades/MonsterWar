/*****************************************************************//**
 * @file   ui_interactive.h
 * @brief  可交互元素的基类, 继承自UIElement
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.01.20
 *********************************************************************/

#pragma once
#ifndef UI_INTERACTIVE_H
#define UI_INTERACTIVE_H

#include "ui_element.h"
#include "state/ui_state.h"
#include "../render/sprite.h"   // 需要引入头文件而不是前置声明（map容器创建时可能会检查内部元素是否有析构定义）
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace engine::core {
    class Context;
}

namespace engine::ui {

/**
* @brief 可交互UI元素的基类,继承自UIElement
*
* 定义了可交互UI元素的通用属性和行为。
* 管理UI状态的切换和交互逻辑。
* 提供事件处理、更新和渲染的虚方法。
*/
class UIInteractive : public UIElement {
public:
    UIInteractive(engine::core::Context& context, const glm::vec2& position = { 0.0f, 0.0f }, const glm::vec2& size = { 0.0f, 0.0f });
    ~UIInteractive() override;

    virtual void clicked() {}                                                               ///< @brief 如果有点击事件，则重写该方法

    void addSprite(std::string_view name, std::unique_ptr<engine::render::Sprite> sprite);  ///< @brief 添加精灵
    void setSprite(std::string_view name);                                                  ///< @brief 设置当前显示的精灵
    void addSound(std::string_view name, std::string_view path);                            ///< @brief 添加音效
    void playSound(std::string_view name);                                                  ///< @brief 播放音效
    // --- Getters and Setters ---
    void setState(std::unique_ptr<engine::ui::state::UIState> state);                       ///< @brief 设置当前状态
    engine::ui::state::UIState* getState() const { return mState.get(); }                   ///< @brief 获取当前状态

    void setInteractive(bool interactive) { mInteractive = interactive; }                   ///< @brief 设置是否可交互
    bool isInteractive() const { return mInteractive; }                                     ///< @brief 获取是否可交互

    // --- 核心方法 ---
    bool handleInput(engine::core::Context& context) override;
    void render(engine::core::Context& context) override;

protected:
	engine::core::Context& mContext;                                                        ///< @brief 可交互元素很可能需要其他引擎组件
	std::unique_ptr<engine::ui::state::UIState> mState;                                     ///< @brief 当前状态
	std::unordered_map<std::string, std::unique_ptr<engine::render::Sprite>> mSprites;      ///< @brief 精灵集合
	std::unordered_map<std::string, std::string> mSounds;                                   ///< @brief 音效集合，key为音效名称，value为音效文件路径
	engine::render::Sprite* mCurrentSprite = nullptr;                                       ///< @brief 当前显示的精灵
	bool mInteractive = true;                                                               ///< @brief 是否可交互
};

} // namespace engine::ui

#endif // !UI_INTERACTIVE_H
