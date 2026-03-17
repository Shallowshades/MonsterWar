/*****************************************************************//**
 * @file   ui_label.h
 * @brief  文本标签UI
 * @version 1.0
 * 
 * @author ShallowShades
 * @date   2026.01.19
 *********************************************************************/

#pragma once

#ifndef UI_LABEL_H
#define UI_LABEL_H

#include <string>
#include <string_view>

#include "ui_element.h"
#include "../utils/math.h"
#include "../render/text_renderer.h"

namespace engine::ui {

/**
* @brief UILabel 类用于创建和管理用户界面中的文本标签
*
* UILabel 继承自 UIElement，提供了文本渲染功能。
* 它可以设置文本内容、字体ID、字体大小和文本颜色。
*
* @note 需要一个文本渲染器来获取和更新文本尺寸。
*/
class UILabel final : public UIElement {
public:
    /**
        * @brief 构造一个UILabel
        *
        * @param text_renderer 文本渲染器
        * @param text 文本内容
        * @param font_id 字体ID
        * @param font_size 字体大小
        * @param text_color 文本颜色
        */
    UILabel(engine::render::TextRenderer& text_renderer,
        std::string_view text,
        std::string_view fontId,
        int fontSize = 16,
        const engine::utils::FColor& textColor = { 1.0f, 1.0f, 1.0f, 1.0f },
        const glm::vec2& position = { 0.0f, 0.0f });

    // --- 核心方法 ---
    void render(engine::core::Context& context) override;

    // --- Setters & Getters ---
    std::string_view getText() const { return mText; }
    std::string_view getFontId() const { return mFontId; }
    int getFontSize() const { return mFontSize; }
    const engine::utils::FColor& getTextFColor() const { return mTextFcolor; }

    void setText(std::string_view text);                                ///< @brief 设置文本内容, 同时更新尺寸
    void setFontId(std::string_view fontId);                            ///< @brief 设置字体ID, 同时更新尺寸
    void setFontSize(int fontSize);                                     ///< @brief 设置字体大小, 同时更新尺寸
    void setTextFColor(const engine::utils::FColor& textFcolor);

private:
	engine::render::TextRenderer& mTextRenderer;                        ///< @brief 需要文本渲染器，用于获取/更新文本尺寸

	std::string mText;                                                  ///< @brief 文本内容    
	std::string mFontId;                                                ///< @brief 字体ID
	int mFontSize;                                                      ///< @brief 字体大小   
	engine::utils::FColor mTextFcolor = { 1.0f, 1.0f, 1.0f, 1.0f };
	/* 可添加其他内容，例如边框、底色 */
};
} // namespace engine::ui

#endif // !UI_LABEL_H
