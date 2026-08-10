/*****************************************************************//**
 * @file   ui_pressed_state.h
 * @brief  按下状态
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.01.20
 *********************************************************************/

#pragma once
#ifndef UI_PRESSED_STATE_H
#define UI_PRESSED_STATE_H

#include "ui_state.h"

namespace engine::ui::state {

/**
* @brief 按下状态
*
* 当鼠标按下UI元素时，会切换到该状态。
*/
class UIPressedState final : public UIState {
    friend class engine::ui::UIInteractive;
public:
    UIPressedState(engine::ui::UIInteractive* owner);
    ~UIPressedState() override;

private:
    void enter() override;
    void update(float deltaTime, engine::core::Context& context) override;
    bool onMouseReleased();   ///< @brief 鼠标释放回调函数（订阅信号触发，非轮询isActionReleased）
};

} // namespace engine::ui::state

#endif // !UI_PRESSED_STATE_H
