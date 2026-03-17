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
*/
class UIHoverState final : public UIState {
	friend class engine::ui::UIInteractive;
public:
	UIHoverState(engine::ui::UIInteractive* owner) : UIState(owner) {}
	~UIHoverState() override = default;

private:
	void enter() override;
	std::unique_ptr<UIState> handleInput(engine::core::Context& context) override;
};

} // namespace engine::ui::state

#endif // !UI_HOVER_H

