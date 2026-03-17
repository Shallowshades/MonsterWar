/*****************************************************************//**
 * @file   ui_button.h
 * @brief  按钮UI元素
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.01.20
 *********************************************************************/

#pragma once
#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "ui_interactive.h"
#include <functional>
#include <utility>

namespace engine::ui {

/**
* @brief 按钮UI元素
*
* 继承自UIInteractive，用于创建可交互的按钮。
* 支持三种状态：正常、悬停、按下。
* 支持回调函数，当按钮被点击时调用。
*/
class UIButton final : public UIInteractive {
public:
    /**
        * @brief 构造函数
        * @param normalSpriteId 正常状态的精灵ID
        * @param hoverSpriteId 悬停状态的精灵ID
        * @param pressedSpriteId 按下状态的精灵ID
        * @param position 位置
        * @param size 大小
        * @param callback 回调函数
        */
    UIButton(engine::core::Context& context,
        std::string_view normalSpriteId,
        std::string_view hoverSpriteId,
        std::string_view pressedSpriteId,
        const glm::vec2& position = { 0.0f, 0.0f },
        const glm::vec2& size = { 0.0f, 0.0f },
        std::function<void()> callback = nullptr);
    ~UIButton() override = default;

    void clicked() override;     ///< @brief 重写基类方法，当按钮被点击时调用回调函数

    void setCallback(std::function<void()> callback) { mCallback = std::move(callback); }   ///< @brief 设置点击回调函数
    std::function<void()> getCallback() const { return mCallback; }                         ///< @brief 获取点击回调函数
private:
	std::function<void()> mCallback;        ///< @brief 可自定义的函数（函数包装器）
};

} // namespace engine::ui

#endif // !UI_BUTTON_H
