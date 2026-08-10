/*****************************************************************//**
 * @file   ui_hover_state.h
 * @brief  悬停状态
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.01.20
 *********************************************************************/

#pragma once
#ifndef UI_HOVER_H
#define UI_HOVER_H

#include "ui_state.h"

namespace engine::ui::state {
/**
* @brief 悬停状态
*
* 当鼠标悬停在UI元素上时，会切换到该状态。
* @note 通过订阅 InputManager 的"mouse_left"按下信号来感知按下事件（事件回调，非轮询）。
*/
class UIHoverState final : public UIState {
	friend class engine::ui::UIInteractive;
public:
	UIHoverState(engine::ui::UIInteractive* owner);
	~UIHoverState() override;

private:
	void enter() override;
	void update(float deltaTime, engine::core::Context& context) override;

	bool onMousePressed();   ///< @brief 鼠标按下回调函数（订阅信号触发，非轮询isActionPressed）
};

} // namespace engine::ui::state

#endif // !UI_HOVER_H

