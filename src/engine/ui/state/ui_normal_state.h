/*****************************************************************//**
 * @file   ui_normal_state.h
 * @brief  正常状态
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.01.20
 *********************************************************************/

#pragma once
#ifndef UI_NORMAL_STATE_H
#define UI_NORMAL_STATE_H

#include "ui_state.h"

namespace engine::ui::state {

/**
* @brief 正常状态
*
* 正常状态是UI元素的默认状态。
*/
class UINormalState final : public UIState {
    friend class engine::ui::UIInteractive;
public:
    UINormalState(engine::ui::UIInteractive* owner) : UIState(owner) {}
    ~UINormalState() override = default;

private:
    void enter() override;
    std::unique_ptr<UIState> handleInput(engine::core::Context& context) override;
};

} // namespace engine::ui::state

#endif // !UI_NORMAL_STATE_H
