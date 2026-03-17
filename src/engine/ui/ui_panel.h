/*****************************************************************//**
 * @file   ui_panel.h
 * @brief  用于分组其他UI元素的容器UI元素
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.01.19
 *********************************************************************/

#pragma once

#ifndef UI_PANEL_H
#define UI_PANEL_H

#include "ui_element.h"
#include <optional>
#include "../utils/math.h"

namespace engine::ui {

/**
 * 分组UI的容器.
 * 
 * Panel 通常用于布局和组织
 * 可以选择是否绘制背景色(纯色)
 */
class UIPanel final : public UIElement {
public:
    /**
	 * @brief 构造一个Panel
	 *
	 * @param position Panel 的局部位置
	 * @param size Panel 的大小
	 * @param background_color 背景色
	 */
    explicit UIPanel(const glm::vec2& position = { 0.0f, 0.0f },
        const glm::vec2& size = { 0.0f, 0.0f },
        const std::optional<engine::utils::FColor>& backgroundColor = std::nullopt);

    void setBackgroundColor(const std::optional<engine::utils::FColor>& backgroundColor) { mBackgroundColor = backgroundColor; }
    const std::optional<engine::utils::FColor>& getBackgroundColor() const { return mBackgroundColor; }

	void render(engine::core::Context& context) override;
private:
	std::optional<engine::utils::FColor> mBackgroundColor;
};
}

#endif // !UI_PANEL_H
