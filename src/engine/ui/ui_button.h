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
#include "../render/image.h"
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
        * @brief 构造函数（直接传入三态图片）
        * @param normalImage 正常状态的图片
        * @param hoverImage 悬停状态的图片
        * @param pressedImage 按下状态的图片
        * @param position 位置
        * @param size 大小
        * @param callback 点击回调函数
        * @note 适用于图片已从配置文件构造好的场景（如UIConfig中的肖像框）
        */
    UIButton(engine::core::Context& context,
        engine::render::Image normalImage,
        engine::render::Image hoverImage,
        engine::render::Image pressedImage,
        const glm::vec2& position = { 0.0f, 0.0f },
        const glm::vec2& size = { 0.0f, 0.0f },
        std::function<void()> callback = nullptr);
    ~UIButton() override = default;

    void clicked() override;        ///< @brief 点击回调（由按下状态的mouse_left释放信号触发）
    void hover_enter() override;    ///< @brief 悬停进入回调（由悬停状态enter触发）
    void hover_leave() override;    ///< @brief 悬停离开回调（由悬停状态update触发）

    void setClickCallback(std::function<void()> callback);       ///< @brief 设置点击回调函数
    void setHoverEnterCallback(std::function<void()> callback);  ///< @brief 设置悬停进入回调函数
    void setHoverLeaveCallback(std::function<void()> callback);  ///< @brief 设置悬停离开回调函数
    void setHoverSound(std::string_view filePath);               ///< @brief 设置悬停音效（注册到"hover"）
    void setClickSound(std::string_view filePath);               ///< @brief 设置点击音效（注册到"pressed"）
private:
	std::function<void()> mClickCallback;        ///< @brief 点击回调
	std::function<void()> mHoverEnterCallback;   ///< @brief 悬停进入回调
	std::function<void()> mHoverLeaveCallback;   ///< @brief 悬停离开回调
};

} // namespace engine::ui

#endif // !UI_BUTTON_H
